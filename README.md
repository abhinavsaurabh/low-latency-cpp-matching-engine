# Low-Latency C++ Matching Engine

A modern C++ matching engine and in-memory order book simulator focused on price-time priority execution, pre-trade risk controls, deterministic testing, and benchmark tooling.

## Overview

This project models the core of a simple exchange-style matching engine for a single symbol. It accepts orders, validates them against basic risk limits, matches aggressive flow against the resting book, and maintains top-of-book state.

The current implementation is intentionally compact and easy to follow:

- Modern C++20 codebase with a small, readable surface area
- Price-time priority matching for limit and market-style flows
- Support for `add`, `cancel`, and `modify`
- Pre-trade checks for order size and absolute position limits
- Demo executable, unit tests, and a simple latency benchmark

## What It Demonstrates

- Order book design and matching engine control flow
- Exchange-style event generation for accepts, executions, cancels, modifies, and rejects
- Separation of concerns across matching, risk, benchmarking, and demo layers
- A clean foundation for extending into more realistic trading infrastructure

## Current Feature Set

- Single-symbol matching engine
- Limit orders
- Market orders implemented as IOC-style aggressive orders
- Price-time priority execution
- Best bid / best ask snapshotting
- `cancel` and `modify` support
- Basic pre-trade risk checks
- Deterministic demo flow
- Unit tests for core behavior, FIFO sequencing, market/IOC flow, and modify/risk paths
- Benchmark executable reporting `p50`, `p99`, and `p99.9`

## Project Layout

- `include/hft/`: public interfaces and shared types
- `src/`: matching engine, order book, and risk implementation
- `apps/demo_main.cpp`: sample order flow and printed book snapshots
- `tests/order_book_tests.cpp`: unit tests for core paths
- `bench/engine_bench.cpp`: simple latency benchmark harness
- `CMakeLists.txt`: project build configuration

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

The generated `build/` directory is not intended to be committed.

## Run

```bash
./build/hft_demo
./build/hft_tests
./build/hft_bench
```

## Demo Output

`hft_demo` submits a short sequence of orders and prints:

- acceptance and execution events
- trade counterparts via `match_id`
- best bid / best ask after each command

This makes it easy to follow how resting liquidity changes as new orders arrive.

## Limitations

This project is not yet a production-grade exchange core. The current version is focused on clarity, correctness, and extensibility rather than full market-microstructure coverage or aggressive low-latency optimization.

Areas still open for improvement include:

- stronger correctness coverage around edge cases
- deeper market data and wire protocol support
- improved data structures for cache locality
- more realistic threading and queueing models
- documented performance analysis with profiler output

Recent hardening work includes:

- preserving stable order lookup after partial fills at the same price level
- enforcing risk validation on order modifications
- extending tests for FIFO behavior, IOC semantics, and rejection paths

## Good Next Steps

- Replace `std::map` price levels with flatter symbol-specialized containers
- Add a binary order-entry or market-data protocol
- Introduce a single-writer engine thread with an ingress queue
- Add replay-driven tests for more complex fill and cancel sequences
- Document measured benchmark results and optimization tradeoffs
