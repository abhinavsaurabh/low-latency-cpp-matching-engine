#include "hft/order_book.hpp"

#include <algorithm>
#include <limits>

namespace hft {

namespace {

using BidLevels = OrderBook::BidLevels;
using AskLevels = OrderBook::AskLevels;
using RestingOrder = OrderBook::RestingOrder;

Event execution_event(
  const Order& aggressor,
  const Order& resting,
  Price price,
  Quantity quantity,
  bool for_aggressor
);

template <typename Levels>
void match_order(
  Levels& opposite,
  const Order& order,
  Quantity& remaining,
  std::unordered_map<OrderId, OrderBook::OrderLocator>& locators,
  std::vector<Event>& events
) {
  auto crossable = [&](Price best_price) {
    return order.side == Side::Buy ? order.price >= best_price : order.price <= best_price;
  };

  while (remaining > 0U && !opposite.empty()) {
    auto best_it = opposite.begin();
    if (!crossable(best_it->first)) {
      break;
    }

    auto& queue = best_it->second;
    while (remaining > 0U && !queue.empty()) {
      auto& resting = queue.front();
      const auto executed = std::min(remaining, resting.open_quantity);
      remaining -= executed;
      resting.open_quantity -= executed;

      events.push_back(execution_event(order, resting.order, resting.order.price, executed, true));
      events.push_back(execution_event(order, resting.order, resting.order.price, executed, false));

      if (resting.open_quantity == 0U) {
        locators.erase(resting.order.order_id);
        queue.pop_front();
      }
    }

    if (queue.empty()) {
      opposite.erase(best_it);
    }
  }
}

Event accepted_event(const Order& order) {
  return Event {
    .type = EventType::Accepted,
    .order_id = order.order_id,
    .resting_order_id = 0,
    .trader_id = order.trader_id,
    .side = order.side,
    .price = order.price,
    .quantity = order.quantity,
    .reject_reason = RejectReason::None,
  };
}

Event rejected_event(const Order& order, RejectReason reason) {
  return Event {
    .type = EventType::Rejected,
    .order_id = order.order_id,
    .resting_order_id = 0,
    .trader_id = order.trader_id,
    .side = order.side,
    .price = order.price,
    .quantity = order.quantity,
    .reject_reason = reason,
  };
}

Event cancelled_event(const Order& order, Quantity open_quantity) {
  return Event {
    .type = EventType::Cancelled,
    .order_id = order.order_id,
    .resting_order_id = 0,
    .trader_id = order.trader_id,
    .side = order.side,
    .price = order.price,
    .quantity = open_quantity,
    .reject_reason = RejectReason::None,
  };
}

Event modified_event(const Order& order) {
  return Event {
    .type = EventType::Modified,
    .order_id = order.order_id,
    .resting_order_id = 0,
    .trader_id = order.trader_id,
    .side = order.side,
    .price = order.price,
    .quantity = order.quantity,
    .reject_reason = RejectReason::None,
  };
}

Event execution_event(
  const Order& aggressor,
  const Order& resting,
  Price price,
  Quantity quantity,
  bool for_aggressor
) {
  return Event {
    .type = EventType::Executed,
    .order_id = for_aggressor ? aggressor.order_id : resting.order_id,
    .resting_order_id = for_aggressor ? resting.order_id : aggressor.order_id,
    .trader_id = for_aggressor ? aggressor.trader_id : resting.trader_id,
    .side = for_aggressor ? aggressor.side : resting.side,
    .price = price,
    .quantity = quantity,
    .reject_reason = RejectReason::None,
  };
}

}  // namespace

CommandResult OrderBook::add(const Order& order) {
  if (order.quantity == 0U) {
    return CommandResult {
      .accepted = false,
      .reject_reason = RejectReason::InvalidQuantity,
      .events = {rejected_event(order, RejectReason::InvalidQuantity)},
    };
  }
  if (order.type == OrderType::Limit && order.price <= 0) {
    return CommandResult {
      .accepted = false,
      .reject_reason = RejectReason::InvalidPrice,
      .events = {rejected_event(order, RejectReason::InvalidPrice)},
    };
  }
  if (locators_.contains(order.order_id)) {
    return CommandResult {
      .accepted = false,
      .reject_reason = RejectReason::DuplicateOrderId,
      .events = {rejected_event(order, RejectReason::DuplicateOrderId)},
    };
  }
  return order.type == OrderType::Limit ? add_limit(order) : add_market(order);
}

CommandResult OrderBook::cancel(OrderId order_id) {
  const auto locator_it = locators_.find(order_id);
  if (locator_it == locators_.end()) {
    return CommandResult {.accepted = false, .reject_reason = RejectReason::UnknownOrderId, .events = {}};
  }

  const auto locator = locator_it->second;
  if (locator.side == Side::Buy) {
    auto level_it = bids_.find(locator.price);
    if (level_it == bids_.end() || locator.offset >= level_it->second.size()) {
      return CommandResult {.accepted = false, .reject_reason = RejectReason::UnknownOrderId, .events = {}};
    }

    auto order = level_it->second[locator.offset].order;
    const auto open_quantity = level_it->second[locator.offset].open_quantity;
    level_it->second.erase(level_it->second.begin() + static_cast<std::ptrdiff_t>(locator.offset));
    locators_.erase(locator_it);
    rebuild_locators_at_price(locator.side, locator.price);
    prune_empty_levels();

    return CommandResult {
      .accepted = true,
      .reject_reason = RejectReason::None,
      .events = {cancelled_event(order, open_quantity)},
    };
  }

  auto level_it = asks_.find(locator.price);
  if (level_it == asks_.end() || locator.offset >= level_it->second.size()) {
    return CommandResult {.accepted = false, .reject_reason = RejectReason::UnknownOrderId, .events = {}};
  }

  auto order = level_it->second[locator.offset].order;
  const auto open_quantity = level_it->second[locator.offset].open_quantity;
  level_it->second.erase(level_it->second.begin() + static_cast<std::ptrdiff_t>(locator.offset));
  locators_.erase(locator_it);
  rebuild_locators_at_price(locator.side, locator.price);
  prune_empty_levels();

  return CommandResult {
    .accepted = true,
    .reject_reason = RejectReason::None,
    .events = {cancelled_event(order, open_quantity)},
  };
}

CommandResult OrderBook::modify(OrderId order_id, Price new_price, Quantity new_quantity, SequenceNumber new_sequence) {
  const auto locator_it = locators_.find(order_id);
  if (locator_it == locators_.end()) {
    return CommandResult {.accepted = false, .reject_reason = RejectReason::UnknownOrderId, .events = {}};
  }
  if (new_quantity == 0U) {
    return CommandResult {.accepted = false, .reject_reason = RejectReason::InvalidQuantity, .events = {}};
  }
  if (new_price <= 0) {
    return CommandResult {.accepted = false, .reject_reason = RejectReason::InvalidPrice, .events = {}};
  }

  const auto locator = locator_it->second;
  Order order {};
  if (locator.side == Side::Buy) {
    auto level_it = bids_.find(locator.price);
    if (level_it == bids_.end() || locator.offset >= level_it->second.size()) {
      return CommandResult {.accepted = false, .reject_reason = RejectReason::UnknownOrderId, .events = {}};
    }
    order = level_it->second[locator.offset].order;
    level_it->second.erase(level_it->second.begin() + static_cast<std::ptrdiff_t>(locator.offset));
  } else {
    auto level_it = asks_.find(locator.price);
    if (level_it == asks_.end() || locator.offset >= level_it->second.size()) {
      return CommandResult {.accepted = false, .reject_reason = RejectReason::UnknownOrderId, .events = {}};
    }
    order = level_it->second[locator.offset].order;
    level_it->second.erase(level_it->second.begin() + static_cast<std::ptrdiff_t>(locator.offset));
  }
  locators_.erase(locator_it);
  rebuild_locators_at_price(locator.side, locator.price);
  prune_empty_levels();

  order.price = new_price;
  order.quantity = new_quantity;
  order.sequence = new_sequence;

  auto result = add_limit(order);
  if (result.accepted) {
    result.events.insert(result.events.begin(), modified_event(order));
  }
  return result;
}

BookSnapshot OrderBook::snapshot() const {
  return BookSnapshot {
    .best_bid = top_level(bids_),
    .best_ask = top_level(asks_),
  };
}

std::size_t OrderBook::order_count() const noexcept {
  return locators_.size();
}

CommandResult OrderBook::add_limit(const Order& order) {
  CommandResult result {.accepted = true, .reject_reason = RejectReason::None, .events = {accepted_event(order)}};
  Quantity remaining = order.quantity;

  if (order.side == Side::Buy) {
    match_order(asks_, order, remaining, locators_, result.events);
  } else {
    match_order(bids_, order, remaining, locators_, result.events);
  }

  if (remaining == 0U || order.tif == TimeInForce::IOC) {
    return result;
  }

  auto resting = order;
  resting.quantity = remaining;
  if (order.side == Side::Buy) {
    auto& queue = bids_[resting.price];
    queue.push_back(RestingOrder {.order = resting, .open_quantity = remaining});
    locators_[resting.order_id] = OrderLocator {.side = resting.side, .price = resting.price, .offset = queue.size() - 1U};
  } else {
    auto& queue = asks_[resting.price];
    queue.push_back(RestingOrder {.order = resting, .open_quantity = remaining});
    locators_[resting.order_id] = OrderLocator {.side = resting.side, .price = resting.price, .offset = queue.size() - 1U};
  }
  return result;
}

CommandResult OrderBook::add_market(const Order& order) {
  auto market_as_limit = order;
  market_as_limit.price = order.side == Side::Buy ? std::numeric_limits<Price>::max() : std::numeric_limits<Price>::min();
  market_as_limit.tif = TimeInForce::IOC;
  return add_limit(market_as_limit);
}

void OrderBook::prune_empty_levels() {
  for (auto it = bids_.begin(); it != bids_.end();) {
    it = it->second.empty() ? bids_.erase(it) : std::next(it);
  }
  for (auto it = asks_.begin(); it != asks_.end();) {
    it = it->second.empty() ? asks_.erase(it) : std::next(it);
  }
}

void OrderBook::rebuild_locators_at_price(Side side, Price price) {
  if (side == Side::Buy) {
    auto level_it = bids_.find(price);
    if (level_it == bids_.end()) {
      return;
    }
    for (std::size_t index = 0; index < level_it->second.size(); ++index) {
      const auto& resting = level_it->second[index];
      locators_[resting.order.order_id] = OrderLocator {.side = side, .price = price, .offset = index};
    }
    return;
  }

  auto level_it = asks_.find(price);
  if (level_it == asks_.end()) {
    return;
  }
  for (std::size_t index = 0; index < level_it->second.size(); ++index) {
    const auto& resting = level_it->second[index];
    locators_[resting.order.order_id] = OrderLocator {.side = side, .price = price, .offset = index};
  }
}

Quantity OrderBook::aggregate_quantity(const std::deque<RestingOrder>& orders) noexcept {
  Quantity total = 0;
  for (const auto& resting : orders) {
    total = static_cast<Quantity>(total + resting.open_quantity);
  }
  return total;
}

std::optional<Level> OrderBook::top_level(const BidLevels& levels) {
  if (levels.empty()) {
    return std::nullopt;
  }
  const auto& [price, orders] = *levels.begin();
  return Level {.price = price, .quantity = aggregate_quantity(orders), .order_count = orders.size()};
}

std::optional<Level> OrderBook::top_level(const AskLevels& levels) {
  if (levels.empty()) {
    return std::nullopt;
  }
  const auto& [price, orders] = *levels.begin();
  return Level {.price = price, .quantity = aggregate_quantity(orders), .order_count = orders.size()};
}

}  // namespace hft
