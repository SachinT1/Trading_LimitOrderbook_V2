

#include <vector>
#include <list>
using Price = std::int32_t;
using Quantity = std::uint32_t;
using OrderId = std::uint64_t;
using OrderIds = std::vector<OrderId>;

class Order;
using OrderPointer = Order* ;
using OrderPointers = std::list<OrderPointer>;