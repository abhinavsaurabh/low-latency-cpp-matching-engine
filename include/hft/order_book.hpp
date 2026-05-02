#pragma once

#include <deque>
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

#include "hft/execution.hpp"
#include "hft/order.hpp"

namespace hft {

class OrderBook {
 public:
  OrderBook() = default;

  struct RestingOrder {
    Order order {};
    Quantity open_quantity {};
  };

  using BidLevels = std::map<Price, std::deque<RestingOrder>, std::greater<>>;
  using AskLevels = std::map<Price, std::deque<RestingOrder>, std::less<>>;

  struct OrderLocator {
    Side side {Side::Buy};
    Price price {};
    std::size_t offset {};
  };

  CommandResult add(const Order& order);
  CommandResult cancel(OrderId order_id);
  CommandResult modify(OrderId order_id, Price new_price, Quantity new_quantity, SequenceNumber new_sequence);

  [[nodiscard]] BookSnapshot snapshot() const;
  [[nodiscard]] std::size_t order_count() const noexcept;

 private:
  CommandResult add_limit(const Order& order);
  CommandResult add_market(const Order& order);
  void prune_empty_levels();
  void rebuild_locators_at_price(Side side, Price price);
  static Quantity aggregate_quantity(const std::deque<RestingOrder>& orders) noexcept;
  static std::optional<Level> top_level(const BidLevels& levels);
  static std::optional<Level> top_level(const AskLevels& levels);

  BidLevels bids_ {};
  AskLevels asks_ {};
  std::unordered_map<OrderId, OrderLocator> locators_ {};
};

}  // namespace hft
