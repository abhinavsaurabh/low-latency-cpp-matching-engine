#pragma once

#include "hft/order_book.hpp"
#include "hft/risk.hpp"

namespace hft {

class MatchingEngine {
 public:
  explicit MatchingEngine(RiskLimits limits = {});

  CommandResult submit(const AddOrderRequest& request);
  CommandResult cancel(const CancelOrderRequest& request);
  CommandResult modify(const ModifyOrderRequest& request);

  [[nodiscard]] const OrderBook& book() const noexcept { return book_; }
  [[nodiscard]] const RiskManager& risk() const noexcept { return risk_; }

 private:
  SequenceNumber next_sequence() noexcept;
  void apply_fills(const CommandResult& result, const Order& aggressor);

  SequenceNumber sequence_ {1};
  OrderBook book_ {};
  RiskManager risk_ {};
};

}  // namespace hft
