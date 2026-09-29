#pragma once

#include <string>
#include <iostream>
#include <iomanip>

using namespace std;

// ─────────────────────────────────────────────
//  Order  –  represents one buy or sell order
//  Plain values only — no pointers, no manual memory management.
// ─────────────────────────────────────────────
class Order {
public:
    long long order_id;
    int       quantity;
    long long price;      // in integer ticks/cents (e.g. 20.70 -> 2070)
    char      side;       // 'B' = buy, 'S' = sell
    long      seconds;    // timestamp in seconds since midnight (for FIFO tie-breaking)
    string    timestamp;  // original "HH:MM:SS" string (for printing)

    Order();
    Order(long long id, int qty, long long p, char s, long secs, const string& ts = "");

    void print() const;
    bool isEarlierThan(const Order& other) const;

    static string formatPrice(long long p);
};