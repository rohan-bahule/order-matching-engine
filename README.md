# Order Matching Engine

A simplified limit order book matching engine that simulates how exchanges match
buy and sell orders, using **price-time priority** (the same core principle real
exchanges like NSE/NASDAQ use).

## What it does

- Reads a stream of orders (and cancels) from `data.csv`
- Matches incoming buy/sell orders against the best available resting price
- Supports partial fills, multiple price levels, and order cancellation
- Writes every executed trade to `output.csv`
- Prints a live trade log and an end-of-run summary to the console

## How to build and run

```bash
make rebuild
./sim
```

## Input format (`data.csv`)

Pipe-delimited rows: `side | order_id | symbol | eq_type | timestamp | price | quantity`

- `side = B` or `S` → a new buy/sell order (price and quantity required)
- `side = C` → cancel an existing resting order (price/quantity can be left blank)

```
B|1|ABC|EQ|10:00:00|55.00|100
S|2|ABC|EQ|10:00:01|50.00|100
C|1|ABC|EQ|10:00:02||
```

## Output format (`output.csv`)

One row per executed trade: `buy_order_id, sell_order_id, quantity, price`

The file is **truncated (overwritten) on every run**, so it only ever reflects
the most recent run's trades.

## Core data structures

```cpp
map<double, deque<Order>> buy_orders;   // price -> FIFO queue of resting buy orders
map<double, deque<Order>> sell_orders;  // price -> FIFO queue of resting sell orders
unordered_map<long long, char> order_location;  // order_id -> 'B' or 'S' (for fast cancel lookup)
```

- **`map<double, deque<Order>>`** — a `map` keeps price levels sorted automatically,
  so the best price is always `begin()` (lowest sell) or the last element (highest buy).
  A `deque` at each price level preserves insertion order, giving FIFO (first-come,
  first-served) ordering among orders resting at the same price.
- **`unordered_map<long long, char>`** — lets `cancelOrder()` immediately know which
  side (buy/sell) an order_id is sitting on, instead of having to search both books.

No raw pointers, `new`/`delete`, or manual memory management are used anywhere —
every `Order` is stored and copied by value.

## Matching algorithm: price-time priority

An incoming **buy** order matches against the **lowest-priced** resting sell order,
as long as that price is **≤** the buy's price. An incoming **sell** order matches
against the **highest-priced** resting buy order, as long as that price is **≥**
the sell's price. Among orders at the same price, the earliest-placed order is
matched first (FIFO).

Partial fills are handled in three cases:
1. `incoming.qty == resting.qty` → both orders fully consumed
2. `incoming.qty < resting.qty` → resting order partially filled, its remaining
   quantity stays in the book
3. `incoming.qty > resting.qty` → resting order fully consumed, the incoming
   order loops to look for the next-best price level

If no resting order can be matched, the incoming order is parked in its own
book at its price.

## Time complexity

| Operation | Complexity | Why |
|---|---|---|
| Insert a new order (no match) | O(log n) | one `map` insertion, where n = number of distinct price levels |
| Match against best price | O(log n) per price level crossed, O(1) per order within a level | `map::begin()`/`rbegin()` is O(1); erasing an empty price level is O(log n) |
| Cancel an order | O(1) average | `unordered_map` lookup for side, then a scan within that side's price levels |
| Print full book | O(n) | n = total resting orders |

## Design decisions and trade-offs

- **Why `map` + `deque` instead of a single sorted vector?** A sorted vector would
  need O(n) insertion to keep it ordered. `map` gives O(log n) insertion and
  automatic ordering by price, and a `deque` per price level keeps FIFO ordering
  without re-sorting on every insert.
- **Why scan price levels on cancel instead of storing an exact price in the index?**
  Cancellation is rare compared to matching in a realistic order flow, so trading
  a small amount of cancel-time cost for a simpler index (`order_id -> side` only)
  was a deliberate simplicity-for-performance trade-off.
- **Why does a cancel for a non-existent order just fail silently (logged, not
  thrown)?** Real market data feeds can have out-of-order or duplicate messages
  (e.g. a cancel arriving for an order that was already fully matched). The engine
  treats this as a normal, expected case rather than an error — it logs and moves on,
  rather than crashing or queuing a "pending cancel" for later.
- **Why store `Order` by value everywhere instead of using pointers?** At this
  scale, the simplicity and safety of value semantics (no manual memory management,
  no dangling pointers, no ownership ambiguity) outweighs the minor copying cost.
  A production system handling millions of orders/sec would likely use a different
  memory layout, but that's a different problem than the one this project solves.

## What this project deliberately does not include

- **Multithreading** — real matching engines are typically single-threaded by
  design, since price-time priority depends on strict sequential processing.
  Threading the matcher itself would risk *incorrect* matching, not just be
  harder to implement.
- **Persistence/database** — the order book is intentionally in-memory only,
  matching how real low-latency matching engines operate; persistence is a
  separate concern handled by downstream systems (e.g. trade reporting, audit logs).