////
// @file storage.cpp
// @brief
// storage manager
//
// @author yangtao
// @email yangtaojay@gmail.com
//
#include "storage.h"
#include <assert.h>

static const uint32_t NUM_BLOCK = 16;

StorageManager::StorageManager(const char* path) : meta(NULL) {
    int res = file.create(path, NUM_BLOCK);
    if (res == 0) {
        // TODO: failed to create file
    }
    else if (res == 1) {
        if (!initFile()) {
            //TODO: error
        }
    }
    meta = (MetaBlock*)getBlock(0);
}

StorageManager::~StorageManager() {
    if (!save()) {
        exit(1);
    }
    for (auto& x : buffers) {
        bufferManager.freeBlock(x.second);
    }
}

void* StorageManager::getBlock(uint32_t index) {
    RecordBlock* block = buffers[index];
    if (block == NULL || block->header.index != index) {
        block = (RecordBlock*)bufferManager.allocateBlock();
        if (!file.readBlock(index, block))
            return NULL;
        buffers[index] = block;
    }
    if (block != NULL)
        block->header.reserved = 1;
    return block;
}

const void* StorageManager::readBlock(uint32_t index) {
    RecordBlock* block = buffers[index];
    if (block == NULL || block->header.index != index) {
        block = (RecordBlock*)bufferManager.allocateBlock();
        if (!file.readBlock(index, block))
            return NULL;
        buffers[index] = block;
    }
    if (block != NULL)
        block->header.reserved = 1;
    return block;
}

// TODO: remove `index`
void* StorageManager::getFreeBlock() {
    if (meta->free == meta->count && meta->freeList == 0) {
        meta->count *= 2;
        if (!file.resize(meta->count)) {
            // TODO: resize failed
            return NULL;
        }

        // free block list
        for (uint32_t i = meta->count / 2; i < meta->count; i++) {
            RecordBlock* block = (RecordBlock*)getBlock(i);
            if (block == NULL)
                return NULL;
            block->header.magic = MAGIC_NUM;
            block->header.index = i;
            block->header.type = BLOCK_TYPE_FREE;

        }
        //if (!save()) {
        //    // TODO: error handle
        //    fprintf(stderr, "write file failed");
        //}
    }

    RecordBlock* b = NULL;
    // free list
    if (meta->freeList != 0) {
        b = (RecordBlock*)readBlock(meta->freeList);
        if (b != NULL)
            meta->freeList = b->header.next;
    }
    else {
        b = (RecordBlock*)readBlock(meta->free);
        if (b != NULL) {
            b->header.index = meta->free;
            meta->free++;
        }
    }
    if (b == NULL)
        return NULL;
    b->header.magic = MAGIC_NUM;
    b->header.reserved = 1;
    b->header.next = 0;
    return b;
}

void StorageManager::freeBlock(uint32_t index) {
    RecordBlock* b = (RecordBlock*)readBlock(index);
    b->header.next = meta->freeList;
    meta->freeList = index;
}

bool StorageManager::save() {
    for (auto& x : buffers) {
        if (x.second->header.reserved == 1) {
            if (!file.writeBlock(x.second->header.index, x.second))
                return false;
            x.second->header.reserved = 0;
        }
    }
    return true;
}

uint32_t StorageManager::getIndexOfRoot() {
    return meta->root;
}

void StorageManager::setIndexOfRoot(uint32_t i) {
    assert(i > 0 && i < meta->count);
    meta->root = i;
}

bool StorageManager::initFile() {
    meta = (MetaBlock*)getBlock(0);
    if (meta == NULL)
        return false;

    meta->header.magic = MAGIC_NUM;
    meta->header.type = BLOCK_TYPE_META;
    meta->header.index = 0;
    meta->header.next = 0;
    meta->root = 0;
    meta->free = 1;
    meta->count = NUM_BLOCK;
    meta->freeList = 0;

    /*for (uint32_t i = 1; i < NUM_BLOCK; i++) {
        RecordBlock *block = (RecordBlock*)getBlock(i);
        if (block == NULL) return false;
        block->header.magic = MAGIC_NUM;
        block->header.index = i;
        block->header.type = BLOCK_TYPE_FREE;
    }*/
    //save();
    return true;
}
