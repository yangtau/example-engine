////
// @file buffer.cpp
// @brief
// buffer manager
//
// @author yangtao
// @email yangtaojay@gmail.com
#include "buffer.h"
#include <stdlib.h>
#include "Windows.h"

//BufferManager::BufferManager() : freeList(NULL) {
//    pools = allocPool();
//}

BufferManager::~BufferManager() {
    /*while (pools != NULL) {
        BufferPool* p = pools->next;
        free(pools);
        pools = p;
    }*/
    while (freeList != NULL) {
        BufferBlock *p = freeList->next;
        _aligned_free(freeList);
        freeList = p;
    }
}

//BufferPool* BufferManager::allocPool() {
//    BufferPool* pool = (BufferPool*)malloc(POOL_SIZE);
//    if (pool == NULL) return NULL;
//    pool->next = NULL;
//    // align with BLOCK_SIZE
//    BufferBlock* block =
//        (BufferBlock*)(((uint64_t)pool + sizeof(BufferPool) + BLOCK_SIZE - 1) /
//            BLOCK_SIZE * BLOCK_SIZE);
//    for (int i = 0; i < BLOCK_NUM; i++) {
//        block->next = freeList;
//        freeList = block;
//        block = (BufferBlock*)((uint8_t*)block + BLOCK_SIZE);
//    }
//    return pool;
//}

void* BufferManager::allocBlock() {
    if (freeList == NULL) {
        /*BufferPool *p = allocPool();
        if (p == NULL) return NULL;
        p->next = BufferManager::pools;
        BufferManager::pools = p;*/
        BufferBlock *p = (BufferBlock *)_aligned_malloc(BLOCK_SIZE, BLOCK_SIZE);
        if (p != NULL) VirtualLock(p, BLOCK_SIZE);
        return p;
    }
    BufferBlock* t = freeList;
    freeList = freeList->next;
    return t;
}

void BufferManager::freeBlock(void* _block) {
    BufferBlock *block = (BufferBlock *)_block;
    block->next = freeList;
    freeList = block;
}