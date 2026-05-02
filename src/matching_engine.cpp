#include "hft/matching_engine.hpp"

namespace hft {

MatchingEngine::MatchingEngine(RiskLimits limits) : risk_(limits) {}

CommandResult MatchingEngine::submit(const AddOrderRequest& request) {
  auto order = request.order;
  order.sequence = next_sequence();

  const auto risk_reject = risk_.validate(order);
  if (risk_reject != RejectReason::None) {
    return CommandResult {
      .accepted = false,
      .reject_reason = risk_reject,
      .events = {Event {
        .type = EventType::Rejected,
        .order_id = order.order_id,
        .resting_order_id = 0,
        .trader_id = order.trader_id,
        .side = order.side,
        .price = order.price,
        .quantity = order.quantity,
        .reject_reason = risk_reject,
      }},
    };
  }

  auto result = book_.add(order);
  if (result.accepted) {
    apply_fills(result, order);
  }
  return result;
}

CommandResult MatchingEngine::cancel(const CancelOrderRequest& request) {
  return book_.cancel(request.order_id);
}

CommandResult MatchingEngine::modify(const ModifyOrderRequest& request) {
  return book_.modify(request.order_id, request.new_price, request.new_quantity, next_sequence());
}

SequenceNumber MatchingEngine::next_sequence() noexcept {
  return sequence_++;
}

void MatchingEngine::apply_fills(const CommandResult& result, const Order& aggressor) {
  for (const auto& event : result.events) {
    if (event.type != EventType::Executed) {
      continue;
    }
    if (event.order_id == aggressor.order_id) {
      risk_.on_fill(aggressor, event.quantity);
    } else {
      risk_.on_fill(event.trader_id, event.side, event.quantity);
    }
  }
}

}  // namespace hft
