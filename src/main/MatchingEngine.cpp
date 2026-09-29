#include "../include/MatchingEngine.hpp"
#include <iostream>
#include <iomanip>

using namespace std;

// ─────────────────────────────────────────────
//  MatchingEngine – implementation
// ─────────────────────────────────────────────

MatchingEngine::MatchingEngine(const string& output_filename) {
    seen_order_ids.reserve(1000000);
    output_file.open(output_filename, ios::trunc);  // overwrite (clear) on every run
    if (!output_file.is_open())
        cerr << "Warning: Could not open output file: " << output_filename << "\n";
}

// ── Record a trade ────────────────────────────────────────────────────────────
void MatchingEngine::recordMatch(const Order& buy, const Order& sell, long long execution_price) {
    order_book.addMatch(buy, sell, execution_price);
    order_book.writeToCSV(output_file, buy, sell, execution_price);
    order_book.displayLastMatch();
    total_volume += buy.quantity;
}

// ── Entry point ───────────────────────────────────────────────────────────────
void MatchingEngine::processOrder(Order order) {
    if ((order.side != 'B' && order.side != 'S') || order.quantity <= 0 || order.price <= 0) {
        cerr << "Warning: Invalid order rejected (ID: " << order.order_id << ")\n";
        return;
    }
    if (!seen_order_ids.insert(order.order_id).second) {
        cerr << "Warning: Duplicate order ID " << order.order_id << " rejected\n";
        return;
    }
    total_orders++;
    if (order.side == 'B')
        matchBuyOrder(order);
    else
        matchSellOrder(order);
}

// ── Match an incoming BUY order against resting SELL orders ──────────────────
// Sweeps sell_orders from lowest price upward (best price first).
// Matches any sell price that is <= the buy's price.
void MatchingEngine::matchBuyOrder(Order order) {
    while (order.quantity > 0) {
        if (sell_orders.empty()) {
            buy_orders[order.price].push_back(order);
            order_location[order.order_id] = 'B';
            return;
        }

        // begin() = lowest sell price (best for the buyer)
        auto best = sell_orders.begin();

        if (best->first > order.price) {
            // Cheapest sell is still too expensive — park the buy
            buy_orders[order.price].push_back(order);
            order_location[order.order_id] = 'B';
            return;
        }

        deque<Order>& level   = best->second;
        Order&        resting = level.front();  // FIFO: oldest sell at this price

        if (resting.quantity == order.quantity) {
            // ── Full match ────────────────────────────────────────────────
            recordMatch(order, resting, resting.price);
            order_location.erase(resting.order_id);
            level.pop_front();
            if (level.empty()) sell_orders.erase(best);
            order.quantity = 0;

        } else if (resting.quantity > order.quantity) {
            // ── Partial fill: sell has more qty than buy ───────────────────
            Order matchedSell    = resting;
            matchedSell.quantity = order.quantity;
            recordMatch(order, matchedSell, resting.price);
            resting.quantity -= order.quantity;
            order.quantity = 0;

        } else {
            // ── Partial fill: buy has more qty than sell ───────────────────
            Order matchedBuy    = order;
            matchedBuy.quantity = resting.quantity;
            recordMatch(matchedBuy, resting, resting.price);
            order.quantity -= resting.quantity;
            order_location.erase(resting.order_id);
            level.pop_front();
            if (level.empty()) sell_orders.erase(best);
            // loop continues — buy still has remaining quantity to match
        }
    }
}

// ── Match an incoming SELL order against resting BUY orders ──────────────────
// Sweeps buy_orders from highest price downward (best price first).
// Matches any buy price that is >= the sell's price.
void MatchingEngine::matchSellOrder(Order order) {
    while (order.quantity > 0) {
        if (buy_orders.empty()) {
            sell_orders[order.price].push_back(order);
            order_location[order.order_id] = 'S';
            return;
        }

        // prev(end()) = highest buy price (best for the seller)
        auto best = prev(buy_orders.end());

        if (best->first < order.price) {
            // Highest buy bid is still too low — park the sell
            sell_orders[order.price].push_back(order);
            order_location[order.order_id] = 'S';
            return;
        }

        deque<Order>& level   = best->second;
        Order&        resting = level.front();  // FIFO: oldest buy at this price

        if (resting.quantity == order.quantity) {
            // ── Full match ────────────────────────────────────────────────
            recordMatch(resting, order, resting.price);
            order_location.erase(resting.order_id);
            level.pop_front();
            if (level.empty()) buy_orders.erase(best);
            order.quantity = 0;

        } else if (resting.quantity > order.quantity) {
            // ── Partial fill: buy has more qty than sell ───────────────────
            Order matchedBuy    = resting;
            matchedBuy.quantity = order.quantity;
            recordMatch(matchedBuy, order, resting.price);
            resting.quantity -= order.quantity;
            order.quantity = 0;

        } else {
            // ── Partial fill: sell has more qty than buy ───────────────────
            Order matchedSell    = order;
            matchedSell.quantity = resting.quantity;
            recordMatch(resting, matchedSell, resting.price);
            order.quantity -= resting.quantity;
            order_location.erase(resting.order_id);
            level.pop_front();
            if (level.empty()) buy_orders.erase(best);
        }
    }
}

// ── Cancel a resting order ────────────────────────────────────────────────────
bool MatchingEngine::cancelOrder(long long order_id) {
    auto loc_it = order_location.find(order_id);
    if (loc_it == order_location.end()) {
        cout << "Cancel failed: Order " << order_id
             << " not found (already matched or never existed)\n";
        return false;
    }

    char   side = loc_it->second;
    auto& book  = (side == 'B') ? buy_orders : sell_orders;

    for (auto it = book.begin(); it != book.end(); ++it) {
        deque<Order>& level = it->second;
        for (auto dit = level.begin(); dit != level.end(); ++dit) {
            if (dit->order_id == order_id) {
                level.erase(dit);
                if (level.empty()) book.erase(it);
                order_location.erase(loc_it);
                total_cancelled++;
                cout << "Order " << order_id << " cancelled\n";
                return true;
            }
        }
    }
    order_location.erase(loc_it);
    return false;
}

// ── Display helpers ───────────────────────────────────────────────────────────
void MatchingEngine::printBuyOrders() const {
    for (const auto& [price, level] : buy_orders)
        for (const auto& o : level) o.print();
}

void MatchingEngine::printSellOrders() const {
    for (const auto& [price, level] : sell_orders)
        for (const auto& o : level) o.print();
}

// ── Summary ───────────────────────────────────────────────────────────────────
void MatchingEngine::printSummary(double elapsed_seconds) const {
    int unmatched_buy = 0;
    for (const auto& [price, level] : buy_orders)
        unmatched_buy += (int)level.size();

    int unmatched_sell = 0;
    for (const auto& [price, level] : sell_orders)
        unmatched_sell += (int)level.size();

    int total_matched = (int)order_book.size();

    string line(44, '=');
    cout << "\n";
    cout << "+" << line << "+\n";
    cout << "|" << "         ORDER BOOK SUMMARY             " << "|\n";
    cout << "+" << line << "+\n";
    cout << "|  " << left << setw(28) << "Total Orders Processed"  << ": "
         << right << setw(10) << total_orders    << "  |\n";
    cout << "|  " << left << setw(28) << "Total Trades Matched"    << ": "
         << right << setw(10) << total_matched   << "  |\n";
    cout << "|  " << left << setw(28) << "Total Orders Cancelled"  << ": "
         << right << setw(10) << total_cancelled << "  |\n";
    cout << "|  " << left << setw(28) << "Unmatched Buy Orders"    << ": "
         << right << setw(10) << unmatched_buy   << "  |\n";
    cout << "|  " << left << setw(28) << "Unmatched Sell Orders"   << ": "
         << right << setw(10) << unmatched_sell  << "  |\n";
    cout << "|  " << left << setw(28) << "Total Volume Traded"     << ": "
         << right << setw(10) << total_volume    << "  |\n";
    cout << "|  " << left << setw(28) << "Execution Time"          << ": "
         << right << setw(8)  << fixed << setprecision(2)
         << elapsed_seconds << " sec  |\n";
    cout << "+" << line << "+\n";
}