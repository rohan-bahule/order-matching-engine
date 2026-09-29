#pragma once

#include "Order.hpp"
#include "OrderBook.hpp"
#include <map>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <string>

using namespace std;

// ──────────────────────────────────────────────────────────────────────────────
//  MatchingEngine
//
//  Matching algorithm: PRICE-TIME PRIORITY (FIFO)
//  ─────────────────────────────────────────────
//  BUY  order matches the lowest-priced resting SELL order whose price <= buy price.
//  SELL order matches the highest-priced resting BUY order whose price >= sell price.
//  Among orders at the same price, the earliest-placed order matches first (FIFO).
//
//  Data structures:
//    map<long long, deque<Order>>  buy_orders / sell_orders
//      map  → prices (in ticks) kept sorted automatically → O(log n) insert / best-price lookup
//      deque → FIFO order within each price level → O(1) front access
//    unordered_map<long long, char>  order_location
//      order_id → 'B' or 'S'  → O(1) lookup for fast cancellation
//    unordered_set<long long>  seen_order_ids
//      historical set of all accepted order IDs for duplicate-ID protection
// ──────────────────────────────────────────────────────────────────────────────
class MatchingEngine {
private:
    map<long long, deque<Order>> buy_orders;
    map<long long, deque<Order>> sell_orders;

    unordered_map<long long, char> order_location;  // order_id -> 'B' or 'S'
    unordered_set<long long>       seen_order_ids;  // historical set of all accepted order IDs

    OrderBook order_book;
    ofstream  output_file;

    int       total_orders    = 0;
    int       total_cancelled = 0;
    long long total_volume    = 0;

    void recordMatch(const Order& buy, const Order& sell, long long execution_price);

public:
    explicit MatchingEngine(const string& output_filename);

    void processOrder(Order order);
    bool cancelOrder(long long order_id);

    void matchBuyOrder(Order order);
    void matchSellOrder(Order order);

    void printBuyOrders()  const;
    void printSellOrders() const;
    void printSummary(double elapsed_seconds) const;
};