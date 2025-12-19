cmake_minimum_required(VERSION 3.15)

project(OrderBookProject CXX)

include(FetchContent)
FetchContent_Declare(
    fmt_repo
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 9.1.0
)
FetchContent_MakeAvailable(fmt_repo)
add_executable(OrderBook main.cpp orderbook.cpp)
target_compile_features(OrderBook PRIVATE cxx_std_20)
target_link_libraries(OrderBook PRIVATE fmt)
target_compile_options(OrderBook PRIVATE -fsanitize=address -O1 -g)
target_link_options(OrderBook PRIVATE -fsanitize=address)
