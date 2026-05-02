#pragma once

#include <cstdint>
#include <string_view>

namespace hft {

using OrderId = std::uint64_t;
using Price = std::int64_t;
using Quantity = std::uint32_t;
using SymbolId = std::uint32_t;
using TraderId = std::uint32_t;
using SequenceNumber = std::uint64_t;

enum class Side : std::uint8_t {
  Buy,
  Sell
};

enum class OrderType : std::uint8_t {
  Limit,
  Market
};

enum class TimeInForce : std::uint8_t {
  Day,
  IOC
};

enum class RejectReason : std::uint8_t {
  None,
  DuplicateOrderId,
  UnknownOrderId,
  InvalidQuantity,
  InvalidPrice,
  PositionLimitExceeded,
  OrderSizeLimitExceeded
};

constexpr std::string_view to_string(Side side) noexcept {
  return side == Side::Buy ? "BUY" : "SELL";
}

constexpr std::string_view to_string(OrderType type) noexcept {
  return type == OrderType::Limit ? "LIMIT" : "MARKET";
}

constexpr std::string_view to_string(RejectReason reason) noexcept {
  switch (reason) {
    case RejectReason::None:
      return "NONE";
    case RejectReason::DuplicateOrderId:
      return "DUPLICATE_ORDER_ID";
    case RejectReason::UnknownOrderId:
      return "UNKNOWN_ORDER_ID";
    case RejectReason::InvalidQuantity:
      return "INVALID_QUANTITY";
    case RejectReason::InvalidPrice:
      return "INVALID_PRICE";
    case RejectReason::PositionLimitExceeded:
      return "POSITION_LIMIT_EXCEEDED";
    case RejectReason::OrderSizeLimitExceeded:
      return "ORDER_SIZE_LIMIT_EXCEEDED";
  }
  return "UNKNOWN";
}

}  // namespace hft
