#pragma once

#include <list>
#include <exception>
#include "OrderType.h"
#include "Side.h"
#include "Usings.h"
#include "MemoryPool.h"
#include "Constants.h"
#include <format>





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
            throw std::logic_error(std::format("Order ({}) cannot be filled for more than its remaining quantity.", GetOrderId()));

        remainingQuantity_ -= quantity;
    }
    void setQuantity(Quantity Q){
        remainingQuantity_ = Q;
    }
    void ToGoodTillCancel(Price price) 
    { 
        if (GetOrderType() != OrderType::Market)
            throw std::logic_error(std::format("Order ({}) cannot have its price adjusted, only market orders can.", GetOrderId()));

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
    static MemoryPool orderpool;
    static MemoryPool& GetPool(){
        return orderpool;
    }
};

inline MemoryPool Order::orderpool{1000000,sizeof(Order)};

inline void* Order::operator new(std::size_t size){
    if(size != sizeof(Order)){
        return ::operator new(size);
    }
    return GetPool().allocate();
}

inline void* Order::operator new(std::size_t size,void* ptr){
    return ptr;
}

inline void Order::operator delete(void* ptr,std::size_t size){
    if(size != sizeof(Order)){
        ::operator delete(ptr);
        return;
    }
    GetPool().deallocate(ptr);
}

inline void Order::operator delete(void* ptr){
    if(ptr){
        GetPool().deallocate(ptr);
    }
}

inline OrderPointer Order::CreateOrder(OrderType orderType, OrderId orderId, Side side, Price price, Quantity quantity){
    return new Order(orderType,orderId,side,price,quantity);
}

