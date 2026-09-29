#include "../include/OrderBook.hpp"
#include <iostream>
#include <iomanip>

using namespace std;

// ─────────────────────────────────────────────
//  OrderBook – implementation
// ─────────────────────────────────────────────

void OrderBook::addMatch(const Order& buy, const Order& sell, long long execution_price) {
    matches.push_back({buy, sell, execution_price});
}

void OrderBook::displayLastMatch() const {
    if (matches.empty()) return;
    const auto& m = matches.back();
    cout << left
         << setw(12) << m.buy.order_id
         << setw(12) << m.sell.order_id
         << setw(8)  << m.buy.quantity
         << Order::formatPrice(m.execution_price)
         << "\n";
}

void OrderBook::writeToCSV(ofstream& file, const Order& buy, const Order& sell, long long execution_price) const {
    (void)sell;
    file << buy.order_id  << ","
         << sell.order_id << ","
         << buy.quantity  << ","
         << Order::formatPrice(execution_price) << "\n";
}