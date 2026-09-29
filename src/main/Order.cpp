#include "../include/Order.hpp"

// ─────────────────────────────────────────────
//  Order – implementation
// ─────────────────────────────────────────────

Order::Order()
    : order_id(0), quantity(0), price(0), side('B'), seconds(0) {}

Order::Order(long long id, int qty, long long p, char s, long secs, const string& ts)
    : order_id(id), quantity(qty), price(p), side(s), seconds(secs), timestamp(ts) {}

string Order::formatPrice(long long p) {
    long long dollars = p / 100;
    long long cents   = p % 100;
    string s = to_string(dollars) + ".";
    if (cents < 10) s += "0";
    s += to_string(cents);
    return s;
}

void Order::print() const {
    cout << "Side: "     << side
         << " | ID: "    << order_id
         << " | Time: "  << timestamp
         << " | Price: " << formatPrice(price)
         << " | Qty: "   << quantity
         << "\n";
}

bool Order::isEarlierThan(const Order& other) const {
    return seconds < other.seconds;
}