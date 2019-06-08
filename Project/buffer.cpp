////
// @file buffer.cpp
// @brief
// buffer manager
//
// @author yangtao
// @email yangtaojay@gmail.com
//
#include "buffer.h"
#include "Windows.h"
#include "block.h"
#include <stdlib.h>

void *BufferManager::allocateBlock() {
    void *p = _aligned_malloc(BLOCK_SIZE, BLOCK_SIZE);
    //if (p != NULL) VirtualLock(p, BLOCK_SIZE);
    return p;
}

void BufferManager::freeBlock(void *_block) {
    _aligned_free(_block);
}