cmake_minimum_required(VERSION 3.15)

project(OrderBookProject CXX)

add_executable(OrderBook main.cpp orderbook.cpp)

target_compile_features(OrderBook PRIVATE cxx_std_20)

target_compile_options(OrderBook PRIVATE 
    -fsanitize=address 
    -O1 
    -g
)
target_link_options(OrderBook PRIVATE -fsanitize=address)
