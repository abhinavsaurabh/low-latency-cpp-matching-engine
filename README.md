# Low-Latency Matching Engine in Modern C++

This project is a resume-ready HFT systems showcase built around a single-symbol matching engine, cache-conscious in-memory order book, simple pre-trade risk checks, deterministic test coverage, and a latency benchmark harness.

## Why this project is strong for HFT interviews

- Directly maps to exchange, gateway, and trading infrastructure work
- Lets you discuss price-time priority, latency budgeting, data structures, and risk controls
- Produces concrete resume bullets backed by code and benchmark output

## Current feature set

- Limit and market order support
- `cancel` and `modify` workflows
- Price-time priority matching
- Best bid / best ask snapshotting
- Pre-trade checks for max order size and max absolute position
- Demo executable that replays a short event sequence
- Simple benchmark executable for `p50`, `p99`, and `p99.9` latency
- Unit tests for matching, cancellation, modification, and rejection paths

## Project layout

- `include/hft/`: public headers
- `src/`: matching engine and order book implementation
- `apps/demo_main.cpp`: CLI demonstration
- `tests/order_book_tests.cpp`: unit tests
- `bench/engine_bench.cpp`: latency benchmark harness

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

## Run

```bash
./build/hft_demo
./build/hft_tests
./build/hft_bench
```

## Resume title

**Low-Latency C++ Matching Engine and Order Book Simulator**

## Sample resume bullets

- Designed and implemented a C++20 price-time-priority matching engine supporting limit, market, cancel, and modify order flows.
- Built an in-memory order book with deterministic replay tests and latency benchmarking for `p50`, `p99`, and `p99.9` execution timing.
- Added pre-trade risk controls for position and order-size limits to mirror real trading gateway behavior.

## Good next steps

- Replace `std::map` price levels with flatter, symbol-specialized containers to improve cache locality
- Add multi-symbol routing and a binary market data publisher
- Introduce a lock-free ingress queue between gateway and matching thread
- Capture benchmark runs under `perf`, Instruments, or VTune and document optimization results
