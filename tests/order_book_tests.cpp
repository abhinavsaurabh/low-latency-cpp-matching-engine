#include <cstdlib>
#include <iostream>
#include <string_view>

#include "hft/matching_engine.hpp"

namespace {

using hft::AddOrderRequest;
using hft::MatchingEngine;
using hft::ModifyOrderRequest;
using hft::Order;
using hft::OrderType;
using hft::RejectReason;
using hft::RiskLimits;
using hft::Side;
using hft::TimeInForce;

Order limit_order(
  std::uint64_t order_id,
  std::uint32_t trader_id,
  hft::Side side,
  std::int64_t price,
  std::uint32_t quantity
) {
  return Order {
    .order_id = order_id,
    .trader_id = trader_id,
    .symbol_id = 1,
    .side = side,
    .type = OrderType::Limit,
    .tif = TimeInForce::Day,
    .price = price,
    .quantity = quantity,
    .sequence = 0,
  };
}

void require(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "test failure: " << message << '\n';
    std::exit(1);
  }
}

void test_crossing_trade() {
  MatchingEngine engine;
  require(engine.submit(AddOrderRequest {.order = limit_order(1, 1, Side::Buy, 100, 10)}).accepted, "buy should rest");

  const auto result = engine.submit(AddOrderRequest {.order = limit_order(2, 2, Side::Sell, 100, 4)});
  require(result.accepted, "sell should execute");
  require(result.events.size() == 3U, "accept + two execution events expected");

  const auto snapshot = engine.book().snapshot();
  require(snapshot.best_bid.has_value(), "bid should remain after partial fill");
  require(snapshot.best_bid->price == 100, "best bid price mismatch");
  require(snapshot.best_bid->quantity == 6, "best bid quantity mismatch");
}

void test_cancel() {
  MatchingEngine engine;
  require(engine.submit(AddOrderRequest {.order = limit_order(11, 1, Side::Buy, 101, 9)}).accepted, "order should rest");

  const auto result = engine.cancel(hft::CancelOrderRequest {.order_id = 11});
  require(result.accepted, "cancel should succeed");
  require(engine.book().order_count() == 0U, "book should be empty");
}

void test_modify_reprices_order() {
  MatchingEngine engine;
  require(engine.submit(AddOrderRequest {.order = limit_order(21, 1, Side::Sell, 110, 8)}).accepted, "sell should rest");

  const auto result = engine.modify(ModifyOrderRequest {.order_id = 21, .new_price = 105, .new_quantity = 5});
  require(result.accepted, "modify should succeed");

  const auto snapshot = engine.book().snapshot();
  require(snapshot.best_ask.has_value(), "ask should exist");
  require(snapshot.best_ask->price == 105, "best ask should update");
  require(snapshot.best_ask->quantity == 5, "ask quantity should update");
}

void test_risk_limit_reject() {
  MatchingEngine engine(RiskLimits {.max_order_size = 10, .max_abs_position = 20});
  const auto result = engine.submit(AddOrderRequest {.order = limit_order(31, 7, Side::Buy, 100, 25)});
  require(!result.accepted, "oversized order should reject");
  require(result.reject_reason == RejectReason::OrderSizeLimitExceeded, "unexpected reject reason");
}

}  // namespace

int main() {
  test_crossing_trade();
  test_cancel();
  test_modify_reprices_order();
  test_risk_limit_reject();
  std::cout << "all tests passed\n";
  return 0;
}
