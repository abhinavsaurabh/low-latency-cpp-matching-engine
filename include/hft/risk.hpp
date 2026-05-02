#pragma once

#include <unordered_map>

#include "hft/order.hpp"

namespace hft {

struct RiskLimits {
  Quantity max_order_size {100000};
  std::int64_t max_abs_position {1000000};
};

class RiskManager {
 public:
  explicit RiskManager(RiskLimits limits = {}) : limits_(limits) {}

  [[nodiscard]] RejectReason validate(const Order& order) const noexcept;
  void on_fill(const Order& aggressor, Quantity executed_quantity) noexcept;
  void on_fill(TraderId trader_id, Side side, Quantity executed_quantity) noexcept;
  void on_fill(const Order& aggressor, const Order& resting, Quantity executed_quantity) noexcept;

  [[nodiscard]] std::int64_t position_for(TraderId trader_id) const noexcept;
  [[nodiscard]] const RiskLimits& limits() const noexcept { return limits_; }

 private:
  [[nodiscard]] std::int64_t signed_delta(Side side, Quantity quantity) const noexcept;

  RiskLimits limits_ {};
  std::unordered_map<TraderId, std::int64_t> positions_ {};
};

}  // namespace hft
