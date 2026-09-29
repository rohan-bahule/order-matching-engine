#include "../include/FileIO.hpp"
#include "../include/Order.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cstdio>
#include <vector>
#include <cctype>
#include <climits>

using namespace std;

// ─────────────────────────────────────────────
//  FileIO – implementation
// ─────────────────────────────────────────────

// Convert "HH:MM:SS" string to seconds since midnight
long FileIO::convertToSeconds(const string& time_str) {
    if (time_str.empty()) return -1;
    int hours = -1, minutes = -1, seconds = -1;
    char extra = '\0';
    if (sscanf(time_str.c_str(), "%d:%d:%d%c", &hours, &minutes, &seconds, &extra) != 3) {
        return -1;
    }
    if (hours < 0 || hours > 23 || minutes < 0 || minutes > 59 || seconds < 0 || seconds > 59) {
        return -1;
    }
    return (hours * 3600) + (minutes * 60) + seconds;
}

// Convert decimal price string (e.g. "20.70", "100.00", "100") to integer cents/ticks
long long FileIO::parsePriceToTicks(const string& price_str) {
    if (price_str.empty()) return -1;

    size_t dot_pos = price_str.find('.');
    if (dot_pos == string::npos) {
        // Integer without decimal point (e.g. "100" -> 10000)
        for (char c : price_str) {
            if (!isdigit(static_cast<unsigned char>(c))) return -1;
        }
        try {
            long long dollars = stoll(price_str);
            if (dollars <= 0 || dollars > LLONG_MAX / 100) return -1;
            return dollars * 100;
        } catch (...) {
            return -1;
        }
    } else {
        // Must have exactly two decimal places (e.g. "20.70" -> 2070)
        string int_part  = price_str.substr(0, dot_pos);
        string frac_part = price_str.substr(dot_pos + 1);

        if (int_part.empty() || frac_part.size() != 2) return -1;

        for (char c : int_part) {
            if (!isdigit(static_cast<unsigned char>(c))) return -1;
        }
        for (char c : frac_part) {
            if (!isdigit(static_cast<unsigned char>(c))) return -1;
        }

        try {
            long long dollars = stoll(int_part);
            long long cents   = (frac_part[0] - '0') * 10 + (frac_part[1] - '0');
            if (dollars < 0 || dollars > (LLONG_MAX - cents) / 100) return -1;
            long long ticks = dollars * 100 + cents;
            if (ticks <= 0) return -1;
            return ticks;
        } catch (...) {
            return -1;
        }
    }
}

void FileIO::readCSV(const string& filename, MatchingEngine& engine) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Unable to open file: " << filename << "\n";
        return;
    }

    size_t line_num = 0;
    string line;
    while (getline(file, line)) {
        line_num++;
        if (line.empty()) continue;

        // Manually split on '|', keeping trailing empty fields.
        // (istringstream + getline silently drops a trailing empty field
        //  at end-of-line, which breaks cancel rows like "C|5|||10:00:04||"
        //  where the last fields are intentionally blank.)
        vector<string> fields;
        size_t start = 0;
        while (true) {
            size_t pipe_pos = line.find('|', start);
            if (pipe_pos == string::npos) {
                fields.push_back(line.substr(start));
                break;
            }
            fields.push_back(line.substr(start, pipe_pos - start));
            start = pipe_pos + 1;
        }
        while (fields.size() < 7) fields.push_back("");  // pad missing trailing fields

        const string& side_str     = fields[0];
        const string& order_id_str = fields[1];
        const string& symbol       = fields[2];
        const string& eq_type      = fields[3];
        const string& timestamp    = fields[4];
        const string& price_str    = fields[5];
        const string& qty_str      = fields[6];

        // Explicitly detect and skip CSV header
        if (side_str == "Side" && order_id_str == "OrderID") {
            continue;
        }

        // Validate side: only 'B', 'S', or 'C' allowed
        if (side_str != "B" && side_str != "S" && side_str != "C") {
            cerr << "Warning: Line " << line_num << " rejected - invalid side '" << side_str << "'\n";
            continue;
        }

        // Validate order ID
        long long id = 0;
        try {
            size_t pos = 0;
            id = stoll(order_id_str, &pos);
            if (pos != order_id_str.size()) {
                cerr << "Warning: Line " << line_num << " rejected - invalid order ID\n";
                continue;
            }
        } catch (const exception&) {
            cerr << "Warning: Line " << line_num << " rejected - invalid order ID\n";
            continue;
        }

        if (id <= 0) {
            cerr << "Warning: Line " << line_num << " rejected - order ID must be greater than 0\n";
            continue;
        }

        // Cancellation record
        if (side_str[0] == 'C') {
            engine.cancelOrder(id);
            continue;
        }

        // For B/S orders, all 7 fields must be non-empty
        if (symbol.empty() || eq_type.empty() || timestamp.empty() || price_str.empty() || qty_str.empty()) {
            cerr << "Warning: Line " << line_num << " rejected - missing required field(s)\n";
            continue;
        }

        // Validate price (must be exact positive 2-decimal or integer ticks)
        long long price = parsePriceToTicks(price_str);
        if (price <= 0) {
            cerr << "Warning: Line " << line_num << " rejected - invalid price\n";
            continue;
        }

        // Validate quantity
        int qty = 0;
        try {
            size_t pos = 0;
            double q_val = stod(qty_str, &pos);
            if (pos != qty_str.size() || q_val <= 0.0) {
                cerr << "Warning: Line " << line_num << " rejected - invalid quantity\n";
                continue;
            }
            qty = static_cast<int>(q_val);
            if (qty <= 0) {
                cerr << "Warning: Line " << line_num << " rejected - invalid quantity\n";
                continue;
            }
        } catch (const exception&) {
            cerr << "Warning: Line " << line_num << " rejected - invalid quantity\n";
            continue;
        }

        // Validate timestamp
        long secs = convertToSeconds(timestamp);
        if (secs < 0) {
            cerr << "Warning: Line " << line_num << " rejected - invalid timestamp\n";
            continue;
        }

        char side = side_str[0];
        Order order(id, qty, price, side, secs, timestamp);
        engine.processOrder(order);  // passed by value — simple copy
    }
}