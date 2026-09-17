#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

#include "Order.h"

// A fixed-capacity, open-addressed ID index. Unlike std::unordered_map, it
// allocates all table storage before the matching loop begins.
class FixedOrderIndex {
public:
    explicit FixedOrderIndex(std::size_t max_entries)
        : slots_(table_capacity(max_entries)), mask_(slots_.size() - 1) {}

    bool insert(uint64_t id, Order* order) {
        std::size_t first_deleted = slots_.size();
        const std::size_t start = hash(id);
        for (std::size_t probe = 0; probe < slots_.size(); ++probe) {
            Slot& slot = slots_[(start + probe) & mask_];
            if (slot.state == State::Occupied) {
                if (slot.id == id) {
                    return false;
                }
                continue;
            }
            if (slot.state == State::Deleted) {
                if (first_deleted == slots_.size()) {
                    first_deleted = (start + probe) & mask_;
                }
                continue;
            }

            Slot& destination = first_deleted == slots_.size() ? slot : slots_[first_deleted];
            destination = Slot{id, order, State::Occupied};
            ++size_;
            return true;
        }

        if (first_deleted != slots_.size()) {
            slots_[first_deleted] = Slot{id, order, State::Occupied};
            ++size_;
            return true;
        }
        return false;
    }

    Order* find(uint64_t id) const {
        const std::size_t start = hash(id);
        for (std::size_t probe = 0; probe < slots_.size(); ++probe) {
            const Slot& slot = slots_[(start + probe) & mask_];
            if (slot.state == State::Empty) {
                return nullptr;
            }
            if (slot.state == State::Occupied && slot.id == id) {
                return slot.order;
            }
        }
        return nullptr;
    }

    bool erase(uint64_t id) {
        const std::size_t start = hash(id);
        for (std::size_t probe = 0; probe < slots_.size(); ++probe) {
            Slot& slot = slots_[(start + probe) & mask_];
            if (slot.state == State::Empty) {
                return false;
            }
            if (slot.state == State::Occupied && slot.id == id) {
                slot.order = nullptr;
                slot.state = State::Deleted;
                --size_;
                return true;
            }
        }
        return false;
    }

    std::size_t size() const { return size_; }

private:
    enum class State : unsigned char { Empty, Occupied, Deleted };

    struct Slot {
        uint64_t id = 0;
        Order* order = nullptr;
        State state = State::Empty;
    };

    static std::size_t table_capacity(std::size_t max_entries) {
        if (max_entries > std::numeric_limits<std::size_t>::max() / 2) {
            throw std::length_error("Order index capacity is too large");
        }

        const std::size_t target = max_entries < 2 ? 2 : max_entries * 2;
        std::size_t result = 1;
        while (result < target) {
            if (result > std::numeric_limits<std::size_t>::max() / 2) {
                throw std::length_error("Order index capacity is too large");
            }
            result <<= 1;
        }
        return result;
    }

    static std::size_t hash(uint64_t value) {
        value ^= value >> 33;
        value *= 0xff51afd7ed558ccdULL;
        value ^= value >> 33;
        value *= 0xc4ceb9fe1a85ec53ULL;
        value ^= value >> 33;
        return static_cast<std::size_t>(value);
    }

    std::vector<Slot> slots_;
    std::size_t mask_;
    std::size_t size_ = 0;
};
