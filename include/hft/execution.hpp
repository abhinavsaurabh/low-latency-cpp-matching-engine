#pragma once

#include <optional>
#include <vector>

#include "hft/types.hpp"

namespace hft {

enum class EventType : std::uint8_t {
  Accepted,
  Executed,
  Cancelled,
  Modified,
  Rejected
};

struct Event {
  EventType type {EventType::Accepted};
  OrderId order_id {};
  OrderId resting_order_id {};
  TraderId trader_id {};
  Side side {Side::Buy};
  Price price {};
  Quantity quantity {};
  RejectReason reject_reason {RejectReason::None};
};

struct CommandResult {
  bool accepted {false};
  RejectReason reject_reason {RejectReason::None};
  std::vector<Event> events {};
};

struct Level {
  Price price {};
  Quantity quantity {};
  std::size_t order_count {};
};

struct BookSnapshot {
  std::optional<Level> best_bid {};
  std::optional<Level> best_ask {};
};

}  // namespace hft
