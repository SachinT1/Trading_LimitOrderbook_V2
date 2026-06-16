#pragma once
#include "Order.h"
#include <vector>
#include <cstddef>
#include <iostream>
#include <new>


class OrderMemoryPool{
private:
    union Block{
        char data[sizeof(Order)];
        Block* next;
    };

    const std::size_t block_size_ = sizeof(Order);
    Block* free_list_head_ = nullptr;
    std::vector<char> raw_memory_;
    
public:
    OrderMemoryPool(std::size_t num_blocks): raw_memory_(num_blocks * block_size_){
        char* current_char_ptr = raw_memory_.data(); 
        free_list_head_ = reinterpret_cast<Block*>(current_char_ptr); 
        for (std::size_t i = 0; i < num_blocks - 1; ++i) {
            
            Block* current_block = reinterpret_cast<Block*>(current_char_ptr);
            current_char_ptr += block_size_; 
            current_block->next = reinterpret_cast<Block*>(current_char_ptr);
        }
        Block* last_block = reinterpret_cast<Block*>(current_char_ptr);
        last_block->next = nullptr;
    }

    void* allocate(){
        if(!free_list_head_){
            throw std::bad_alloc();
        }
        void* block = free_list_head_;
        free_list_head_=free_list_head_->next;
        return block;
    }

    void deallocate(void* ptr){
        if(!ptr){
            return;
        }
        Block* block = reinterpret_cast<Block*>(ptr);
        block->next = free_list_head_;
        free_list_head_= block;
    }

    OrderMemoryPool(const OrderMemoryPool&) = delete;
    OrderMemoryPool& operator=(const OrderMemoryPool&) = delete;
    //to invalidate copy operations on this object
};



using OrderPointer = Order* ;
//OrderDeleter should not be written in shared_ptr<> defn
//std::shared_ptr uses a runtime constructor argument std::unique_ptr uses a compile-time deleter template argument

using OrderPointers = std::list<OrderPointer>;