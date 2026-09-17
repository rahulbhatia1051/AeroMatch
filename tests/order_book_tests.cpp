#include <cassert>
#include <cstdint>

#include "OrderBook.h"

int main() {
    {
        OrderBook book(4);
        assert(book.add_order(1, 100, 5, false));
        assert(book.add_order(2, 100, 5, false));
        assert(book.add_order(3, 100, 5, true));

        // Orders at the same price execute in arrival order, not LIFO order.
        assert(!book.contains(1));
        assert(book.contains(2));
        assert(book.remaining_quantity(2).value() == 5);
        assert(book.best_ask_price().value() == 100);

        assert(book.add_order(4, 100, 5, true));
        assert(!book.best_ask_price().has_value());
        assert(book.active_order_count() == 0);
    }

    {
        OrderBook book(2);
        assert(book.add_order(7, 101, 3, true));
        assert(!book.add_order(7, 99, 9, false));
        assert(book.remaining_quantity(7).value() == 3);
        assert(!book.add_order(8, 100, 0, false));
        assert(book.cancel_order(7));
        assert(!book.cancel_order(7));
        assert(book.active_order_count() == 0);
    }

    {
        OrderBook book(4);
        assert(book.add_order(1, 100, 5, false));
        assert(book.add_order(2, 101, 5, false));
        assert(book.add_order(3, 101, 7, true));
        assert(!book.contains(1));
        assert(book.remaining_quantity(2).value() == 3);
        assert(book.best_ask_price().value() == 101);
    }

    {
        OrderBook book(2);
        assert(book.add_order(1, 100, 3, false));
        assert(book.add_market_order(2, 5, true));
        // The unfilled two lots are cancelled, never added as a resting bid.
        assert(book.active_order_count() == 0);
        assert(!book.best_bid_price().has_value());
        assert(!book.best_ask_price().has_value());
    }

    {
        OrderBook book(1);
        assert(book.add_order(1, 100, 1, false));
        assert(!book.add_order(2, 100, 1, false));
        assert(book.cancel_order(1));
        // Cancelled capacity is immediately returned to the pre-allocated pool.
        assert(book.add_order(2, 100, 1, false));
        assert(book.contains(2));
    }

    {
        OrderBook book(2);
        for (uint64_t id = 1; id <= 32; ++id) {
            assert(book.add_order(id, 100 + id, 1, false));
            assert(book.cancel_order(id));
        }
        assert(book.active_order_count() == 0);
    }

    return 0;
}
