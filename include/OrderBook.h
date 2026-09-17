#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "FixedOrderIndex.h"
#include "MemoryPool.h"

struct PriceLevel {
    uint64_t price = 0;
    bool is_buy = false;
    Order* head = nullptr;
    Order* tail = nullptr;
    std::size_t order_count = 0;

    void reset(uint64_t new_price, bool buy) {
        price = new_price;
        is_buy = buy;
        head = nullptr;
        tail = nullptr;
        order_count = 0;
    }

    void append(Order* order) {
        order->prev = tail;
        order->next = nullptr;
        order->level = this;
        order->is_active = true;
        if (tail != nullptr) {
            tail->next = order;
        } else {
            head = order;
        }
        tail = order;
        ++order_count;
    }

    void remove(Order* order) {
        if (order->prev != nullptr) {
            order->prev->next = order->next;
        } else {
            head = order->next;
        }
        if (order->next != nullptr) {
            order->next->prev = order->prev;
        } else {
            tail = order->prev;
        }
        order->prev = nullptr;
        order->next = nullptr;
        order->level = nullptr;
        order->is_active = false;
        --order_count;
    }

    bool empty() const { return order_count == 0; }
};

class OrderBook {
public:
    explicit OrderBook(std::size_t capacity)
        : pool_(capacity), order_index_(capacity), level_storage_(capacity) {
        bids_.reserve(capacity);
        asks_.reserve(capacity);
        free_levels_.reserve(capacity);
        for (std::size_t i = 0; i < capacity; ++i) {
            free_levels_.push_back(&level_storage_[i]);
        }
    }

    OrderBook(const OrderBook&) = delete;
    OrderBook& operator=(const OrderBook&) = delete;

    // Returns false for malformed, duplicate, or over-capacity input. A
    // rejected order has no observable effect on the book.
    bool add_order(uint64_t id, uint64_t price, uint32_t quantity, bool is_buy) {
        return add_order_impl(id, price, quantity, is_buy, false);
    }

    // A market order consumes available liquidity but never rests in the book.
    bool add_market_order(uint64_t id, uint32_t quantity, bool is_buy) {
        return add_order_impl(id, 0, quantity, is_buy, true);
    }

    bool cancel_order(uint64_t id) {
        Order* target = order_index_.find(id);
        if (target == nullptr) {
            return false;
        }

        PriceLevel* level = target->level;
        level->remove(target);
        order_index_.erase(id);
        pool_.release(target);
        if (level->empty()) {
            remove_empty_level(level);
        }
        return true;
    }

    bool contains(uint64_t id) const { return order_index_.find(id) != nullptr; }

    std::optional<uint32_t> remaining_quantity(uint64_t id) const {
        const Order* order = order_index_.find(id);
        if (order == nullptr) {
            return std::nullopt;
        }
        return order->quantity;
    }

    std::optional<uint64_t> best_bid_price() const {
        return bids_.empty() ? std::nullopt : std::optional<uint64_t>(bids_.front()->price);
    }

    std::optional<uint64_t> best_ask_price() const {
        return asks_.empty() ? std::nullopt : std::optional<uint64_t>(asks_.front()->price);
    }

    std::size_t active_order_count() const { return order_index_.size(); }

private:
    bool add_order_impl(uint64_t id, uint64_t price, uint32_t quantity, bool is_buy, bool is_market) {
        if (quantity == 0 || order_index_.find(id) != nullptr) {
            return false;
        }

        Order* incoming = pool_.acquire(id, price, quantity, is_buy);
        if (incoming == nullptr || !order_index_.insert(id, incoming)) {
            if (incoming != nullptr) {
                pool_.release(incoming);
            }
            return false;
        }

        match(incoming, is_market);
        if (incoming->quantity == 0 || is_market) {
            order_index_.erase(incoming->order_id);
            pool_.release(incoming);
            return true;
        }

        PriceLevel* level = find_or_create_level(price, is_buy);
        if (level == nullptr) {
            order_index_.erase(incoming->order_id);
            pool_.release(incoming);
            return false;
        }
        level->append(incoming);
        return true;
    }
    using LevelList = std::vector<PriceLevel*>;

    static LevelList::iterator lower_bound(LevelList& levels, uint64_t price, bool is_buy) {
        if (is_buy) {
            return std::lower_bound(levels.begin(), levels.end(), price,
                [](const PriceLevel* level, uint64_t target) { return level->price > target; });
        }
        return std::lower_bound(levels.begin(), levels.end(), price,
            [](const PriceLevel* level, uint64_t target) { return level->price < target; });
    }

    PriceLevel* find_or_create_level(uint64_t price, bool is_buy) {
        LevelList& levels = is_buy ? bids_ : asks_;
        const auto position = lower_bound(levels, price, is_buy);
        if (position != levels.end() && (*position)->price == price) {
            return *position;
        }
        if (free_levels_.empty()) {
            return nullptr;
        }

        PriceLevel* level = free_levels_.back();
        free_levels_.pop_back();
        level->reset(price, is_buy);
        levels.insert(position, level);
        return level;
    }

    void remove_empty_level(PriceLevel* level) {
        LevelList& levels = level->is_buy ? bids_ : asks_;
        const auto position = lower_bound(levels, level->price, level->is_buy);
        if (position != levels.end() && *position == level) {
            levels.erase(position);
        }
        free_levels_.push_back(level);
    }

    void match(Order* incoming, bool is_market) {
        LevelList& opposite_levels = incoming->is_buy ? asks_ : bids_;
        while (incoming->quantity > 0 && !opposite_levels.empty()) {
            PriceLevel* best_level = opposite_levels.front();
            const bool crosses = is_market || (incoming->is_buy ? incoming->price >= best_level->price
                                                                 : incoming->price <= best_level->price);
            if (!crosses) {
                return;
            }

            Order* resting = best_level->head;
            const uint32_t traded_quantity = std::min(incoming->quantity, resting->quantity);
            incoming->quantity -= traded_quantity;
            resting->quantity -= traded_quantity;

            if (resting->quantity == 0) {
                order_index_.erase(resting->order_id);
                best_level->remove(resting);
                pool_.release(resting);
                if (best_level->empty()) {
                    remove_empty_level(best_level);
                }
            }
        }
    }

    OrderPool pool_;
    FixedOrderIndex order_index_;
    std::vector<PriceLevel> level_storage_;
    std::vector<PriceLevel*> free_levels_;
    LevelList bids_;
    LevelList asks_;
};
