#include "hft/risk.hpp"

namespace hft {

RejectReason RiskManager::validate(const Order& order) const noexcept {
  if (order.quantity == 0U) {
    return RejectReason::InvalidQuantity;
  }
  if (order.type == OrderType::Limit && order.price <= 0) {
    return RejectReason::InvalidPrice;
  }
  if (order.quantity > limits_.max_order_size) {
    return RejectReason::OrderSizeLimitExceeded;
  }

  const auto current = position_for(order.trader_id);
  const auto projected = current + signed_delta(order.side, order.quantity);
  if (projected > limits_.max_abs_position || projected < -limits_.max_abs_position) {
    return RejectReason::PositionLimitExceeded;
  }
  return RejectReason::None;
}

void RiskManager::on_fill(const Order& aggressor, Quantity executed_quantity) noexcept {
  positions_[aggressor.trader_id] += signed_delta(aggressor.side, executed_quantity);
}

void RiskManager::on_fill(TraderId trader_id, Side side, Quantity executed_quantity) noexcept {
  positions_[trader_id] += signed_delta(side, executed_quantity);
}

void RiskManager::on_fill(const Order& aggressor, const Order& resting, Quantity executed_quantity) noexcept {
  positions_[aggressor.trader_id] += signed_delta(aggressor.side, executed_quantity);
  positions_[resting.trader_id] += signed_delta(resting.side, executed_quantity);
}

std::int64_t RiskManager::position_for(TraderId trader_id) const noexcept {
  const auto it = positions_.find(trader_id);
  return it == positions_.end() ? 0 : it->second;
}

std::int64_t RiskManager::signed_delta(Side side, Quantity quantity) const noexcept {
  const auto delta = static_cast<std::int64_t>(quantity);
  return side == Side::Buy ? delta : -delta;
}

}  // namespace hft
