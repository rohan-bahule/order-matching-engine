# Order Matching Engine

A simplified C++ limit order matching engine that simulates how exchanges match buy and sell orders using price-time priority.

## What it does

- Reads a stream of orders and cancellations from `data.csv`
- Matches incoming buy/sell orders against the best available resting price
- Supports price-time priority, partial fills, full fills, and multi-level matching
- Supports order cancellation
- Performs strict input validation and prevents duplicate Order IDs
- Records every executed trade in `output.csv`
- Prints matching activity and an end-of-run summary to the console

## How to build and run

```bash
make rebuild
./sim
```

## Input format (`data.csv`)

Pipe-delimited rows:

```
side | order_id | symbol | eq_type | timestamp | price | quantity
```

- `side = B` or `S` → a new buy/sell order
- `side = C` → cancel an existing resting order
- Price and quantity are required for new buy/sell orders
- Cancel orders do not require price or quantity

Example:

```
B|1|ABC|EQ|10:00:00|55.00|100
S|2|ABC|EQ|10:00:01|50.00|100
C|1|ABC|EQ|10:00:02||
```

## Output format (`output.csv`)

One row per executed trade:

```
buy_order_id,sell_order_id,quantity,execution_price
```

The output file is overwritten on every run and contains only the trades generated during the most recent run.

## Core data structures

```cpp
map<long long, deque<Order>> buy_orders;
map<long long, deque<Order>> sell_orders;
unordered_map<long long, char> order_location;
unordered_set<long long> seen_order_ids;
```

- `map<long long, deque<Order>>` maintains price levels in sorted order.
- The buy book prioritizes the highest price, while the sell book prioritizes the lowest price.
- `deque<Order>` maintains FIFO ordering among orders at the same price.
- `order_location` tracks the side of each currently active order for faster cancellation lookup.
- `seen_order_ids` prevents reuse of Order IDs.
- Prices are stored as integer ticks/cents rather than floating-point values to avoid precision issues.

No raw pointers, `new`/`delete`, or manual memory management are used. Orders are stored by value.

## Matching algorithm: price-time priority

An incoming buy order matches against the lowest-priced resting sell order as long as:

```
sell_price <= buy_price
```

An incoming sell order matches against the highest-priced resting buy order as long as:

```
buy_price >= sell_price
```

Among orders at the same price, the earliest-resting order is matched first (FIFO).

The engine supports:

1. **Full fill**: incoming and resting quantities are equal.
2. **Partial fill of resting order**: incoming quantity is smaller.
3. **Partial fill of incoming order**: resting quantity is smaller, so the incoming order continues matching against the next available price level.

If no further match is possible, the remaining quantity of the incoming order rests in its corresponding book.

## Execution price

Trades execute at the **resting order's price**.

For example:

```
Resting SELL: 100 @ 50.00
Incoming BUY: 100 @ 55.00
```

The trade executes at:

```
50.00
```

Similarly:

```
Resting BUY: 100 @ 55.00
Incoming SELL: 100 @ 50.00
```

The trade executes at:

```
55.00
```