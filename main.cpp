#pragma once
#include "Orderbook.h"
#include "MemoryPool.h"
#include "Order.h"
#include <iostream>
#include <chrono>

using Clock = std::chrono::high_resolution_clock;
using std::chrono::nanoseconds;

int main()
{
    auto st = Clock::now();

    Orderbook orderbook;

    auto t1 = Clock::now();
    auto duration = t1 - st;
    long long microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    std::cout << "Orderbook Initialization Time: " << microseconds << " μs" << std::endl;

    const int NUM_TRIALS = 10000;
    std::vector<nanoseconds> latencies;
    latencies.reserve(NUM_TRIALS);

    for (int i = 0; i < NUM_TRIALS; ++i) {
       
        OrderId new_id = i + 1; 

        auto start = Clock::now();
        
        
        auto trd = orderbook.AddOrder(Order::CreateOrder(OrderType::GoodTillCancel, new_id, Side::Buy, 1000, 10)); 
        
        auto end = Clock::now();
        
        
        latencies.push_back(std::chrono::duration_cast<nanoseconds>(end - start));
    }

    
    std::sort(latencies.begin(), latencies.end());

    // Calculate Average Latency (in nanoseconds)
    long long total_ns = 0;
    for (const auto& ns : latencies) {
        total_ns += ns.count();
    }
    long long average_latency = total_ns / NUM_TRIALS;

    // Find P99 Latency (99th percentile)
    // 99% of 10000 trials = 9900th element (index 9899)
    size_t p99_index = static_cast<size_t>(NUM_TRIALS * 0.99); 
    long long p99_latency = latencies[p99_index].count(); 

    std::cout << "\n--- Performance Metrics ---" << std::endl;
    std::cout << "Average Insertion Latency: " << average_latency << " ns" << std::endl;
    std::cout << "P99 (Worst Case) Latency: " << p99_latency << " ns" << std::endl;








    /*
    auto trd = orderbook.AddOrder(Order::CreateOrder(OrderType::GoodTillCancel,OrderId(1001),Side::Buy,Price(1000),Quantity(50)));
    for(auto it:trd){
        std::cout<<"trd1"<<std::endl;
        auto trd_asks = it.GetAskTrade();
        auto trd_bids = it.GetBidTrade();
        std::cout<<trd_asks.orderId_<<" "<<trd_asks.price_<<" "<<trd_asks.quantity_<<std::endl;
        std::cout<<trd_bids.orderId_<<" "<<trd_bids.price_<<" "<<trd_bids.quantity_<<std::endl;  
    }
    std::cout<<orderbook.Size()<<std::endl;
    
    
    */
    
    
    return 0;
}