#pragma once

#include <string>
#include "MatchingEngine.hpp"

using namespace std;

// ─────────────────────────────────────────────
//  FileIO  –  CSV reader
// ─────────────────────────────────────────────
class FileIO {
public:
    // Read the pipe-delimited input CSV and feed every row into the engine.
    //
    // CSV format (one row):
    //   side | order_id | symbol | eq_type | timestamp | price | quantity
    //
    // side can be:
    //   'B' or 'S'  -> a new buy/sell order  (price and quantity required)
    //   'C'         -> cancel an existing resting order (price/quantity can be blank)
    static void readCSV(const string& filename, MatchingEngine& engine);

    // Convert "HH:MM:SS" string -> seconds since midnight (used for FIFO tie-breaking)
    static long convertToSeconds(const string& time_str);

    // Convert decimal price string -> integer ticks/cents (e.g. "20.70" -> 2070, "100" -> 10000)
    static long long parsePriceToTicks(const string& price_str);
};