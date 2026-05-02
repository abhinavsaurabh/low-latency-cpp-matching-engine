#pragma once

#include "hft/types.hpp"

namespace hft {

struct Order {
  OrderId order_id {};
  TraderId trader_id {};
  SymbolId symbol_id {};
  Side side {Side::Buy};
  OrderType type {OrderType::Limit};
  TimeInForce tif {TimeInForce::Day};
  Price price {};
  Quantity quantity {};
  SequenceNumber sequence {};
};

struct AddOrderRequest {
  Order order {};
};

struct CancelOrderRequest {
  OrderId order_id {};
};

struct ModifyOrderRequest {
  OrderId order_id {};
  Price new_price {};
  Quantity new_quantity {};
};

}  // namespace hft
