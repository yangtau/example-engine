////
// @file buffer.h
// @brief
// buffer manager
// 
// @author yangtao
// @email yangtaojay@gmail.com
// TODO: free used block

#pragma once
#include "block.h"

///
// @brief
// buffer pool
//struct BufferPool {
//    BufferPool* next;
//};
//const uint32_t BLOCK_NUM = 16;
//const uint32_t POOL_SIZE(BLOCK_NUM* BLOCK_SIZE + sizeof(BufferPool));

struct BufferBlock {
    BufferBlock* next;
};

///
// @brief
// buffer manager
class BufferManager {
private:
    //BufferPool* pools;
    BufferBlock* freeList;
    //BufferPool* allocPool();
    // don't implement
    /*BufferManager(BufferBlock const&);
    void operator=(BufferManager const&);*/
public:
    BufferManager() :freeList(NULL) {}
    /*static BufferManager& getInstance() {
        static BufferManager instance;
        return instance;
    }*/
    void* allocBlock();
    void freeBlock(void*);
    ~BufferManager();

};