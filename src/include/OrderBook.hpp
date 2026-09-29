#pragma once

#include "Order.hpp"
#include <vector>
#include <fstream>

using namespace std;

// ─────────────────────────────────────────────
//  OrderMatch  –  a matched buy/sell pair
//  (stored by value — just two plain Order copies)
// ─────────────────────────────────────────────
struct OrderMatch {
    Order     buy;
    Order     sell;
    long long execution_price;
};

// ─────────────────────────────────────────────
//  OrderBook  –  stores all executed trades
// ─────────────────────────────────────────────
class OrderBook {
private:
    vector<OrderMatch> matches;

public:
    void addMatch(const Order& buy, const Order& sell, long long execution_price);
    void displayLastMatch() const;
    void writeToCSV(ofstream& file, const Order& buy, const Order& sell, long long execution_price) const;

    const vector<OrderMatch>& getMatches() const { return matches; }
    size_t size() const { return matches.size(); }
};