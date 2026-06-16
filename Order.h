#pragma once

#include <list>
#include <exception>
#include <fmt/core.h>

#include "OrderType.h"
#include "Side.h"
#include "Usings.h"
#include "Constants.h"


class OrderMemoryPool;
template<typename T> class shared_ptr;
class Order;
using OrderPointer = Order*;


class Order
{
public:
    Order(OrderType orderType, OrderId orderId, Side side, Price price, Quantity quantity)
        : orderType_{ orderType }
        , orderId_{ orderId }
        , side_{ side }
        , price_{ price }
        , initialQuantity_{ quantity }
        , remainingQuantity_{ quantity }
    { }

    Order(OrderId orderId, Side side, Quantity quantity)
        : Order(OrderType::Market, orderId, side, Constants::InvalidPrice, quantity)
    { }

    
    void* operator new(std::size_t size);
    void* operator new(std::size_t size, void* ptr);
    
    //by default the sized destructor is considered. 
    void operator delete(void* ptr, std::size_t size);
    void operator delete(void* ptr); 
    
    OrderId GetOrderId() const { return orderId_; }
    Side GetSide() const { return side_; }
    Price GetPrice() const { return price_; }
    OrderType GetOrderType() const { return orderType_; }
    Quantity GetInitialQuantity() const { return initialQuantity_; }
    Quantity GetRemainingQuantity() const { return remainingQuantity_; }
    Quantity GetFilledQuantity() const { return GetInitialQuantity() - GetRemainingQuantity(); }
    bool IsFilled() const { return GetRemainingQuantity() == 0; }
    void Fill(Quantity quantity)
    {
        if (quantity > GetRemainingQuantity())
            throw std::logic_error(fmt::format("Order ({}) cannot be filled for more than its remaining quantity.", GetOrderId()));

        remainingQuantity_ -= quantity;
    }
    void setQuantity(Quantity Q){
        remainingQuantity_ = Q;
    }
    void ToGoodTillCancel(Price price) 
    { 
        if (GetOrderType() != OrderType::Market)
            throw std::logic_error(fmt::format("Order ({}) cannot have its price adjusted, only market orders can.", GetOrderId()));

        price_ = price;
        orderType_ = OrderType::GoodTillCancel;
    }
    
    static OrderPointer CreateOrder(OrderType orderType, OrderId orderId, Side side, Price price, Quantity quantity);
private:
    OrderType orderType_;
    OrderId orderId_;
    Side side_;
    Price price_;
    Quantity initialQuantity_;
    Quantity remainingQuantity_;

public:
    static OrderMemoryPool pool;
    static OrderMemoryPool& GetPool();
};

