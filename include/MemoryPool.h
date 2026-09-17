#pragma once

#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>

#include "Order.h"

class OrderPool {
public:
    static_assert(std::is_trivially_destructible<Order>::value,
        "OrderPool requires Order to remain trivially destructible");

    explicit OrderPool(std::size_t capacity) : storage_(capacity) {
        free_list_.reserve(capacity);
        for (std::size_t i = 0; i < capacity; ++i) {
            free_list_.push_back(&storage_[i]);
        }
    }

    OrderPool(const OrderPool&) = delete;
    OrderPool& operator=(const OrderPool&) = delete;

    template <typename... Args>
    Order* acquire(Args&&... args) {
        if (free_list_.empty()) {
            return nullptr;
        }

        Order* memory = free_list_.back();
        free_list_.pop_back();
        return new (memory) Order(std::forward<Args>(args)...);
    }

    void release(Order* order) {
        order->is_active = false;
        order->next = nullptr;
        order->prev = nullptr;
        order->level = nullptr;
        free_list_.push_back(order);
    }

private:
    std::vector<Order> storage_;
    std::vector<Order*> free_list_;
};
