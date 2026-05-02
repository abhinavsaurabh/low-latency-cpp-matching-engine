#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <random>
#include <vector>

#include "hft/matching_engine.hpp"

namespace {

using Clock = std::chrono::steady_clock;
using Nanoseconds = std::chrono::nanoseconds;

hft::Order random_limit_order(std::uint64_t order_id, std::mt19937_64& rng) {
  std::uniform_int_distribution<std::uint32_t> trader_dist(1, 100);
  std::uniform_int_distribution<int> side_dist(0, 1);
  std::uniform_int_distribution<int> tick_dist(-20, 20);
  std::uniform_int_distribution<std::uint32_t> qty_dist(1, 1000);

  const auto side = side_dist(rng) == 0 ? hft::Side::Buy : hft::Side::Sell;
  return hft::Order {
    .order_id = order_id,
    .trader_id = trader_dist(rng),
    .symbol_id = 1,
    .side = side,
    .type = hft::OrderType::Limit,
    .tif = hft::TimeInForce::Day,
    .price = 100000 + tick_dist(rng),
    .quantity = qty_dist(rng),
    .sequence = 0,
  };
}

double percentile(std::vector<long long> samples, double p) {
  if (samples.empty()) {
    return 0.0;
  }
  std::sort(samples.begin(), samples.end());
  const auto index = static_cast<std::size_t>(p * static_cast<double>(samples.size() - 1U));
  return static_cast<double>(samples[index]);
}

}  // namespace

int main() {
  constexpr std::size_t iterations = 50000;
  std::mt19937_64 rng(42);
  hft::MatchingEngine engine;
  std::vector<long long> latencies;
  latencies.reserve(iterations);

  for (std::size_t i = 0; i < iterations; ++i) {
    const auto order = random_limit_order(static_cast<std::uint64_t>(i + 1U), rng);
    const auto start = Clock::now();
    [[maybe_unused]] auto result = engine.submit(hft::AddOrderRequest {.order = order});
    const auto end = Clock::now();
    latencies.push_back(std::chrono::duration_cast<Nanoseconds>(end - start).count());
  }

  std::cout << "iterations=" << iterations << '\n';
  std::cout << "p50_ns=" << percentile(latencies, 0.50) << '\n';
  std::cout << "p99_ns=" << percentile(latencies, 0.99) << '\n';
  std::cout << "p999_ns=" << percentile(latencies, 0.999) << '\n';
  return 0;
}
