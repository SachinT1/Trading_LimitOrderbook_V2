#pragma once

#include <vector>
#include <cstddef>
#include <algorithm>
#include <new>


class MemoryPool{
private:
    struct Node{
        Node* next;
    };
    const std::size_t block_size_;
    Node* free_list_head = nullptr;
    std::vector<char>raw_memory_;

public:
    explicit MemoryPool(std::size_t num_blocks,std::size_t req_block_size)
        :block_size_(std::max(req_block_size,sizeof(Node))),
        raw_memory_(block_size_*num_blocks)
    {
        char* currptr = raw_memory_.data();
        free_list_head = reinterpret_cast<Node*>(currptr);

        for(std::size_t i = 0;i<num_blocks-1;++i){
            Node* curr_node = reinterpret_cast<Node*>(currptr);
            currptr += block_size_;
            curr_node->next = reinterpret_cast<Node*>(currptr);
        }
        Node* last_node = reinterpret_cast<Node*>(currptr);
        last_node->next = nullptr;
    }

    void* allocate(){
        if(!free_list_head){
            throw std::bad_alloc();
        }
        void* block = free_list_head;
        free_list_head = free_list_head->next;
        return block;
    }

    void deallocate(void* ptr){
        if(!ptr){
            return;
        }
        Node* node = reinterpret_cast<Node*>(ptr);
        node->next = free_list_head;
        free_list_head = node;
    }

    MemoryPool(const MemoryPool& other) = delete;
    MemoryPool& operator= (const MemoryPool& other) = delete;
};



