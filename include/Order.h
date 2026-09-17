#pragma once

#include <cstdint>

struct PriceLevel;

struct Order {
    uint64_t order_id = 0;
    uint64_t price = 0;
    uint32_t quantity = 0;
    bool is_buy = false;
    bool is_active = false;

    // Links are owned by the price-level FIFO queue, never by the book vector.
    Order* next = nullptr;
    Order* prev = nullptr;
    PriceLevel* level = nullptr;

    Order() = default;

    Order(uint64_t id, uint64_t p, uint32_t q, bool buy)
        : order_id(id), price(p), quantity(q), is_buy(buy), is_active(true) {}
};
