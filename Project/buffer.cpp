////
// @file storage.cpp
// @brief
// buffer manager
//
// @author yangtao
// @email yangtaojay@gmail.com
//
#include "buffer.h"
#include <assert.h>
#include <stdlib.h>
#include "block.h"

BufferManager::BufferManager(MyFile *file, u32 blockSize)
    : file(file), blockSize(blockSize) {}

BufferManager::~BufferManager() {
  // save();
  for (auto &i : pool) {
    _aligned_free(i.second);
  }
}

int BufferManager::save() {
  for (auto &i : pool) {
    // if the block is dirty, `reserved` = 1.
    if (i.second->reserved) {
      i.second->reserved = 0;
      if (!file->writeBlock(i.second, i.first, blockSize)) {
        i.second->reserved = 1;
        return 0;
      }
    }
  }

  return 1;
}

void *BufferManager::getBlock(u32 index, int flag) {
  BlockHeader *block = NULL;
  // find block in buffer
  if (pool.find(index) != pool.end()) {
    block = pool[index];
    assert(block->index == index);
    if (block->reserved == 0) {
      block->reserved = flag;
    }

    return block;
  }

  block = (BlockHeader *)_aligned_malloc(blockSize, blockSize);
  if (block == NULL) return NULL;
  if (!file->readBlock(block, index, blockSize)) {
    _aligned_free(block);
    block = NULL;
  }
  block->reserved = flag;
  pool[index] = block;

  return block;
}
