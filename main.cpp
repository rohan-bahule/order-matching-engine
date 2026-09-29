#include <iostream> 
#include <chrono>

#include "./src/include/MatchingEngine.hpp"
#include "./src/include/FileIO.hpp"

using namespace std;

int main() {
    // ── Start timer ───────────────────────────────────────────────────────────
    auto start = chrono::high_resolution_clock::now();

    // ── Init engine (clears output.csv and opens fresh) ───────────────────────
    MatchingEngine engine("output.csv");

    // ── Feed all orders (and cancels) from the market-data CSV into the engine ─
    FileIO::readCSV("data.csv", engine);

    // ── Print remaining unmatched orders ──────────────────────────────────────
    cout << "\n=== Remaining Unmatched BUY Orders ===\n";
    engine.printBuyOrders();

    cout << "\n=== Remaining Unmatched SELL Orders ===\n";
    engine.printSellOrders();

    // ── Calculate and print summary ───────────────────────────────────────────
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = end - start;
    engine.printSummary(elapsed.count());

    return 0;
}