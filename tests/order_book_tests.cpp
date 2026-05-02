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

Order market_order(std::uint64_t order_id, std::uint32_t trader_id, hft::Side side, std::uint32_t quantity) {
  return Order {
    .order_id = order_id,
    .trader_id = trader_id,
    .symbol_id = 1,
    .side = side,
    .type = OrderType::Market,
    .tif = TimeInForce::IOC,
    .price = 0,
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

void test_locator_stability_after_front_fill() {
  MatchingEngine engine;
  require(engine.submit(AddOrderRequest {.order = limit_order(101, 1, Side::Buy, 100, 5)}).accepted, "first bid should rest");
  require(engine.submit(AddOrderRequest {.order = limit_order(102, 2, Side::Buy, 100, 7)}).accepted, "second bid should rest");

  const auto fill = engine.submit(AddOrderRequest {.order = limit_order(103, 3, Side::Sell, 100, 5)});
  require(fill.accepted, "sell should match first resting bid");

  const auto cancel = engine.cancel(hft::CancelOrderRequest {.order_id = 102});
  require(cancel.accepted, "second bid should still be cancellable after first bid fills");
  require(engine.book().order_count() == 0U, "book should be empty after cancellation");
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

void test_modify_respects_risk_limits() {
  MatchingEngine engine(RiskLimits {.max_order_size = 10, .max_abs_position = 20});
  require(engine.submit(AddOrderRequest {.order = limit_order(41, 7, Side::Buy, 100, 8)}).accepted, "initial order should rest");

  const auto result = engine.modify(ModifyOrderRequest {.order_id = 41, .new_price = 100, .new_quantity = 25});
  require(!result.accepted, "risk-breaching modify should reject");
  require(result.reject_reason == RejectReason::OrderSizeLimitExceeded, "modify reject reason mismatch");

  const auto snapshot = engine.book().snapshot();
  require(snapshot.best_bid.has_value(), "original bid should remain");
  require(snapshot.best_bid->quantity == 8, "rejected modify must not mutate resting order");
}

void test_market_order_is_ioc() {
  MatchingEngine engine;
  require(engine.submit(AddOrderRequest {.order = limit_order(51, 1, Side::Sell, 101, 4)}).accepted, "ask should rest");

  const auto result = engine.submit(AddOrderRequest {.order = market_order(52, 2, Side::Buy, 10)});
  require(result.accepted, "market buy should be accepted");

  const auto snapshot = engine.book().snapshot();
  require(snapshot.best_ask == std::nullopt, "market order should consume resting ask");
  require(snapshot.best_bid == std::nullopt, "unfilled market remainder must not rest");
}

void test_fifo_with_same_price_level() {
  MatchingEngine engine;
  require(engine.submit(AddOrderRequest {.order = limit_order(61, 1, Side::Sell, 105, 3)}).accepted, "first ask should rest");
  require(engine.submit(AddOrderRequest {.order = limit_order(62, 2, Side::Sell, 105, 4)}).accepted, "second ask should rest");

  const auto result = engine.submit(AddOrderRequest {.order = limit_order(63, 3, Side::Buy, 105, 5)});
  require(result.accepted, "crossing buy should execute");
  require(result.events.size() == 5U, "expected accept plus two fills against two resting orders");
  require(result.events[1].resting_order_id == 61, "first fill should target oldest resting order");
  require(result.events[3].resting_order_id == 62, "second fill should target second resting order");

  const auto snapshot = engine.book().snapshot();
  require(snapshot.best_ask.has_value(), "one ask should remain");
  require(snapshot.best_ask->quantity == 2, "remaining second ask quantity mismatch");
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
  test_locator_stability_after_front_fill();
  test_modify_reprices_order();
  test_modify_respects_risk_limits();
  test_market_order_is_ioc();
  test_fifo_with_same_price_level();
  test_risk_limit_reject();
  std::cout << "all tests passed\n";
  return 0;
}
