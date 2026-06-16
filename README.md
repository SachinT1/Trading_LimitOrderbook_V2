# Trading_LimitOrderbook_V2

Latest Updates:
1) Order pointers are no longer shared_ptr<Order> . Shared pointers have an internal atomic reference count and each manipulation which inherently require atomic exchanges, very slowed down verison.
Instead the system now uses raw Order* with manual memory management
2) AddOrderInternal added : the APIs now follow the pattern where there is a standard outer function which takes care of securing the mutex , and the 'internal' functions assume callee has already acquired mutex
3) GetOrderInfos changed to not parse through orders_ and used pre-accumalated info in other Data structures . On this note, data_ has been changed to 2 side specific d.s. askdata_ and biddats_. 

