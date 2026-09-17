#include <iostream>

#include "OrderBook.h"

int main() {
    OrderBook engine(16);

    const bool ask_one = engine.add_order(1, 50000, 10, false);
    const bool ask_two = engine.add_order(2, 50000, 5, false);
    const bool buy = engine.add_order(3, 50100, 12, true);
    const bool cancelled = engine.cancel_order(2);

    std::cout << "AeroMatch deterministic limit-order-book demo\n"
              << "accepted: " << ask_one << ask_two << buy << " | cancelled: " << cancelled << '\n'
              << "active resting orders: " << engine.active_order_count() << '\n';
    return (ask_one && ask_two && buy && cancelled && engine.active_order_count() == 0) ? 0 : 1;
}
