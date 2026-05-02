#include <array>
#include <iostream>
#include <string_view>

#include "hft/matching_engine.hpp"

namespace {

using hft::AddOrderRequest;
using hft::MatchingEngine;
using hft::Order;
using hft::OrderType;
using hft::Side;
using hft::SymbolId;
using hft::TimeInForce;
using hft::TraderId;

Order make_limit(
  std::uint64_t order_id,
  TraderId trader_id,
  SymbolId symbol_id,
  Side side,
  std::int64_t price,
  std::uint32_t quantity
) {
  return Order {
    .order_id = order_id,
    .trader_id = trader_id,
    .symbol_id = symbol_id,
    .side = side,
    .type = OrderType::Limit,
    .tif = TimeInForce::Day,
    .price = price,
    .quantity = quantity,
    .sequence = 0,
  };
}

void print_snapshot(const hft::BookSnapshot& snapshot) {
  std::cout << "BOOK SNAPSHOT\n";
  if (snapshot.best_bid.has_value()) {
    const auto& bid = *snapshot.best_bid;
    std::cout << "  best bid: " << bid.price << " x " << bid.quantity << " (" << bid.order_count << " orders)\n";
  } else {
    std::cout << "  best bid: empty\n";
  }

  if (snapshot.best_ask.has_value()) {
    const auto& ask = *snapshot.best_ask;
    std::cout << "  best ask: " << ask.price << " x " << ask.quantity << " (" << ask.order_count << " orders)\n";
  } else {
    std::cout << "  best ask: empty\n";
  }
}

void print_result(const hft::CommandResult& result) {
  for (const auto& event : result.events) {
    std::cout
      << "event type=" << static_cast<int>(event.type)
      << " order_id=" << event.order_id
      << " match_id=" << event.resting_order_id
      << " side=" << hft::to_string(event.side)
      << " px=" << event.price
      << " qty=" << event.quantity
      << " reject=" << hft::to_string(event.reject_reason)
      << '\n';
  }
}

}  // namespace

int main() {
  MatchingEngine engine;

  const std::array<Order, 4> orders {
    make_limit(1, 1001, 1, Side::Buy, 10000, 500),
    make_limit(2, 2001, 1, Side::Sell, 10040, 300),
    make_limit(3, 2002, 1, Side::Sell, 10000, 200),
    make_limit(4, 1002, 1, Side::Buy, 10010, 250)
  };

  for (const auto& order : orders) {
    auto result = engine.submit(AddOrderRequest {.order = order});
    print_result(result);
    print_snapshot(engine.book().snapshot());
    std::cout << '\n';
  }

  return 0;
}
