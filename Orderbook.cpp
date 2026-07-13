#pragma once
#include "Orderbook.h"
#include "MemoryPool.h"
#include "Order.h"
#include "OrderModify.h"
#include <numeric>
#include <chrono>
#include <ctime>
#include <iostream>

void Orderbook::PruneGoodForDayOrders()
{
	using namespace std::chrono;
	const auto end = hours(16);

	while (true)
	{
		const auto now = system_clock::now();
		const auto now_c = system_clock::to_time_t(now);
		std::tm now_parts;
		localtime_r(&now_c, &now_parts);
		if (now_parts.tm_hour >= end.count())
			now_parts.tm_mday += 1;

		now_parts.tm_hour = end.count();
		now_parts.tm_min = 0;
		now_parts.tm_sec = 0;

		auto next = system_clock::from_time_t(mktime(&now_parts));
		auto till = next - now + milliseconds(100);

		{
			std::unique_lock ordersLock{ordersMutex_};

			if (shutdown_.load(std::memory_order_acquire) || shutdownConditionVariable_.wait_for(ordersLock, till) == std::cv_status::no_timeout)
			{
				// condition 1 shutdown var = true
				// this is the case if orderbook is destroyed( check dstructor fn it sets
				//  shutdown to true)
				//  or condition 2: condition variable has been notified before timeout
				// there is apparently nothing left to process we should just end this sequence of instruction
				// simply return
				return;
			}

			// cancel good for day orders
			for (OrderId id : gfdorders)
			{
				if (orders_.contains(id))
				{
					CancelOrderInternal(id);
				}
			}
			gfdorders.clear();
		}
	}
}

std::thread &Orderbook::getprune()
{
	return ordersPruneThread_;
}

void Orderbook::CancelOrder(OrderId orderId)
{
	std::scoped_lock ordersLock{ordersMutex_};

	CancelOrderInternal(orderId);
}

void Orderbook::CancelOrders(OrderIds orderIds)
{
	std::scoped_lock ordersLock{ordersMutex_};

	for (const auto &orderId : orderIds)
		CancelOrderInternal(orderId);
}

void Orderbook::CancelOrderInternal(OrderId orderId)
{
	if (!orders_.contains(orderId))
		return;

	const auto [orderPtr, iterator] = orders_.at(orderId);

	if (orderPtr->GetSide() == Side::Sell)
	{
		auto price = orderPtr->GetPrice();
		auto &orders = asks_.at(price);
		orders.erase(iterator);
		if (orders.empty())
			asks_.erase(price);
	}
	else
	{
		auto price = orderPtr->GetPrice();
		auto &orders = bids_.at(price);
		orders.erase(iterator);
		if (orders.empty())
			bids_.erase(price);
	}

	OnOrderCancelled(orderPtr);

	orders_.erase(orderId);

	orderPtr->~Order();
	// clean up memory of the inherent order object

	Order::GetPool().deallocate(orderPtr);
	// give back the memory to the orderpool
}

void Orderbook::OnOrderCancelled(OrderPointer order)
{
	UpdateLevelData(order->GetPrice(), order->GetRemainingQuantity(), Action::Remove, order->GetSide());
}

void Orderbook::OnOrderAdded(OrderPointer order)
{
	UpdateLevelData(order->GetPrice(), order->GetInitialQuantity(), Action::Add, order->GetSide());
}

void Orderbook::OnOrderMatched(Price price, Quantity quantity, bool isFullyFilled, Side side)
{
	UpdateLevelData(price, quantity, isFullyFilled ? Action::Remove : Action::Match, side);
}

void Orderbook::UpdateLevelData(Price price, Quantity quantity, Action action, Side side)
{
	auto &data_ = (side == Side::Buy) ? biddata_ : askdata_;

	auto &data = data_[price];

	data.count_ += action == Action::Remove ? -1 : action == Action::Add ? 1
																		 : 0;
	if (action == Action::Remove || action == Action::Match)
	{
		data.quantity_ -= quantity;
	}
	else
	{
		data.quantity_ += quantity;
	}

	if (data.count_ == 0)
		data_.erase(price);
}

bool Orderbook::CanFullyFill(Side side, Price price, Quantity quantity) const
{
	auto &data_ = (side == Side::Buy) ? biddata_ : askdata_;
	if (!CanMatch(side, price))
		return false;

	for (const auto &[levelPrice, levelData] : data_)
	{

		if ((side == Side::Buy && levelPrice > price) ||
			(side == Side::Sell && levelPrice < price))
			continue;

		if (quantity <= levelData.quantity_)
			return true;

		quantity -= levelData.quantity_;
	}
	return false;
}

bool Orderbook::CanMatch(Side side, Price price) const
{
	if (side == Side::Buy)
	{
		if (asks_.empty())
			return false;

		const auto &[bestAsk, _] = *asks_.begin();
		return price >= bestAsk;
	}
	else
	{
		if (bids_.empty())
			return false;

		const auto &[bestBid, _] = *bids_.begin();
		return price <= bestBid;
	}
}

Trades Orderbook::MatchOrders()
{

	Trades trades;

	trades.reserve(64);

	while (true)
	{
		if (bids_.empty() || asks_.empty())
		{
			// one of the sides is empty , cannot match!
			break;
		}

		auto &[bidPrice, bids] = *bids_.begin();
		auto &[askPrice, asks] = *asks_.begin();

		if (bidPrice < askPrice)
			// highest bid is lower than smallest ask
			// cannot match!
			break;

		while (!bids.empty() && !asks.empty())
		{
			auto bid = bids.front();
			auto ask = asks.front();

			Quantity quantity = std::min(bid->GetRemainingQuantity(), ask->GetRemainingQuantity());

			bid->Fill(quantity);
			ask->Fill(quantity);

			// record the trade
			trades.push_back(Trade{
				TradeInfo{bid->GetOrderId(), bid->GetPrice(), quantity},
				TradeInfo{ask->GetOrderId(), ask->GetPrice(), quantity}});

			// update price levels
			OnOrderMatched(bid->GetPrice(), quantity, bid->IsFilled(), Side::Buy);
			OnOrderMatched(ask->GetPrice(), quantity, ask->IsFilled(), Side::Sell);

			// clean up ask/bid
			if (bid->IsFilled())
			{
				bids.pop_front();
				orders_.erase(bid->GetOrderId());
				bid->~Order();
				Order::GetPool().deallocate(bid);
			}
			if (ask->IsFilled())
			{
				asks.pop_front();
				orders_.erase(ask->GetOrderId());
				ask->~Order();
				Order::GetPool().deallocate(ask);
			}
		}

		if (bids.empty())
		{
			biddata_.erase(bidPrice);
			bids_.erase(bidPrice);
		}

		if (asks.empty())
		{
			askdata_.erase(askPrice);
			asks_.erase(askPrice);
		}
	}

	if (!bids_.empty())
	{
		auto &[_, bids] = *bids_.begin();
		auto &order = bids.front();
		if (order->GetOrderType() == OrderType::FillAndKill)

			CancelOrderInternal(order->GetOrderId());
	}

	if (!asks_.empty())
	{
		auto &[_, asks] = *asks_.begin();
		auto &order = asks.front();
		if (order->GetOrderType() == OrderType::FillAndKill)
			CancelOrderInternal(order->GetOrderId());
	}

	return trades;
}

Orderbook::Orderbook() : shutdown_{false}
{
}

void Orderbook::StartBook()
{
	ordersPruneThread_ = std::thread([this]
									 { PruneGoodForDayOrders(); });
}

Orderbook::~Orderbook()
{

	shutdown_.store(true, std::memory_order_release);

	shutdownConditionVariable_.notify_one();

	if (ordersPruneThread_.joinable() == true)
	{
		ordersPruneThread_.join();
	}

	for(auto& [orderId,orderEnt] : orders_){
		auto& [orderptr,orderiter] = orderEnt;
		Order::GetPool().deallocate(orderptr);
	}
	orders_.clear();
	asks_.clear();
	bids_.clear();
	askdata_.clear();
	biddata_.clear();
}

Trades Orderbook::AddOrderInternal(OrderPointer order){
	if (orders_.contains(order->GetOrderId()))
		return {};

	if (order->GetOrderType() == OrderType::Market)
	{

		if (order->GetSide() == Side::Buy && !asks_.empty())
		{
			const auto &[worstAsk, _] = *asks_.rbegin();
			order->ToGoodTillCancel(worstAsk);
		}
		else if (order->GetSide() == Side::Sell && !bids_.empty())
		{
			const auto &[worstBid, _] = *bids_.rbegin();
			order->ToGoodTillCancel(worstBid);
		}
		else
			return {};
	}

	if (order->GetOrderType() == OrderType::FillAndKill && !CanMatch(order->GetSide(), order->GetPrice()))
		return {};

	if (order->GetOrderType() == OrderType::FillOrKill && !CanFullyFill(order->GetSide(), order->GetPrice(), order->GetInitialQuantity()))
		return {};

	OrderPointers::iterator iterator;

	if (order->GetSide() == Side::Buy)
	{
		auto &orders = bids_[order->GetPrice()];
		orders.push_back(order);
		iterator = std::prev(orders.end());
	}
	else
	{
		auto &orders = asks_[order->GetPrice()];
		orders.push_back(order);
		iterator = std::prev(orders.end());
	}

	orders_.insert({order->GetOrderId(), OrderEntry{order, iterator}});
	if (order->GetOrderType() == OrderType::GoodForDay)
	{
		gfdorders.push_back(order->GetOrderId());
	}
	OnOrderAdded(order);
	return MatchOrders();
}

Trades Orderbook::AddOrder(OrderPointer order)
{
	std::scoped_lock ordersLock{ordersMutex_};
	return AddOrderInternal(order);
}

Trades Orderbook::ModifyOrder(OrderModify order)
{

	std::scoped_lock orderLock{ordersMutex_};
	if (!orders_.contains(order.GetOrderId()))
	{
		return {};
	}
	const auto &[existingOrder, _placeh] = orders_.at(order.GetOrderId());
	OrderType originaltype = existingOrder->GetOrderType();

	// optimisation: if this is at the same price
	// and just a quantity downsizing, dont allocate new memory and all
	// just fix the values in place
	if ((existingOrder->GetPrice() == order.GetPrice()) && order.GetQuantity() < existingOrder->GetRemainingQuantity())
	{
		Quantity red = existingOrder->GetRemainingQuantity() - order.GetQuantity();
		existingOrder->setQuantity(order.GetQuantity());
		UpdateLevelData(order.GetPrice(), red, Action::Match, order.GetSide());
		return {};
	}

	// ELSE WE need to consider making new order

	// delete the existing order
	CancelOrderInternal(order.GetOrderId());

	// get a clean new order_ptr for the new order object to be added
	Order *new_order_ptr = order.ToOrderPointer(originaltype);

	// add it to the orderbook
	return AddOrderInternal(new_order_ptr);
}

std::size_t Orderbook::Size() const
{
	std::scoped_lock ordersLock{ordersMutex_};
	return orders_.size();
}

OrderbookLevelInfos Orderbook::GetOrderInfos() const
{
	LevelInfos bidInfos, askInfos;
	bidInfos.reserve(64);
	askInfos.reserve(64);

	for (const auto &[price, orders] : bids_)
		bidInfos.push_back({price, biddata_.at(price).quantity_});

	for (const auto &[price, orders] : asks_)
		askInfos.push_back({price, askdata_.at(price).quantity_});

	return OrderbookLevelInfos{bidInfos, askInfos};
}