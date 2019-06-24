#define _CRT_SECURE_NO_WARNINGS
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
#include <stdlib.h>

static const uint32_t NUM_BLOCK = 16;

StorageManager::StorageManager() : meta(NULL), file(NULL) {}

StorageManager::StorageManager(const char * path) : meta(NULL), file(NULL) {
    open(path);
}

int StorageManager::create(const char * path) {
    //close();
    FILE *file = fopen(path, "wb");
    if (file == NULL) return false;

    MetaBlock *block = (MetaBlock*)bufferManager.allocateBlock();
    block->header.index = 0;
    block->header.type = BLOCK_TYPE_META;
    block->header.magic = MAGIC_NUM;
    block->header.next = 0;
    block->header.last = 0;

    block->count = 1;
    block->free = 0;
    block->root = 0;

    if (fwrite(block, BLOCK_SIZE, 1, file) != 1) {
        bufferManager.freeBlock(block);
        return false;
    }

    fclose(file);

    buffers[0] = (RecordBlock*)block;
    return true;
}

int StorageManager::open(const char * path) {
    //close();
    file = fopen(path, "r+b");
    if (file == NULL) return false;

    // read meta
    meta = (MetaBlock*)getBlock(0);
    if (meta == NULL) return false;
    return true;
}


int StorageManager::close() {
    if (file != NULL) {
        save();
        //fflush(file);
        fclose(file);
        file = NULL;
        for (auto& x : buffers) {
            x.second->header.index = -1;
        }
    }
    return true;
}

bool StorageManager::save() {
    for (auto& x : buffers) {
        if (x.second->header.reserved == 1
            && x.first == x.second->header.index) {
            /*if (!file.writeBlock(x.second->header.index, x.second))
                return false;*/
            if (fseek(file, x.first*BLOCK_SIZE, SEEK_SET) != 0) return false;
            x.second->header.reserved = 0;
            if (fwrite(x.second, BLOCK_SIZE, 1, file) != 1) {
                x.second->header.reserved = 1;
                return false;
            }
            //fflush(file);
        }
    }
    return true;
}



StorageManager::~StorageManager() {
    close();
    //? TODO:
    for (auto& x : buffers) {
        bufferManager.freeBlock(x.second);
    }
}

void* StorageManager::getBlock(uint32_t index) {
    if (index != 0 && index >= meta->count) return NULL;
    RecordBlock* block = buffers[index];
    if (block == NULL) {
        block = (RecordBlock*)bufferManager.allocateBlock();
        /*if (!file.readBlock(index, block))
            return NULL;*/
        if (fseek(file, index * BLOCK_SIZE, SEEK_SET) != 0) return NULL;
        size_t res = fread(block, BLOCK_SIZE, 1, file);
        if (res != 1) {
            bufferManager.freeBlock(block);
            return NULL;
        }
        buffers[index] = block;
    }
    if (block->header.index != index) {
        if (fseek(file, index * BLOCK_SIZE, SEEK_SET) != 0) return NULL;
        size_t res = fread(block, BLOCK_SIZE, 1, file);
        if (res != 1) {
            bufferManager.freeBlock(block);
            return NULL;
        }
    }
    if (block != NULL)
        block->header.reserved = 1;
    return block;
}

//const void* StorageManager::readBlock(uint32_t index) {
//    RecordBlock* block = buffers[index];
//    if (block == NULL || block->header.index != index) {
//        block = (RecordBlock*)bufferManager.allocateBlock();
//        /*if (!file.readBlock(index, block))
//            return NULL;*/
//        buffers[index] = block;
//    }
//    if (block != NULL)
//        block->header.reserved = 1;
//    return block;
//}


void* StorageManager::getFreeBlock() {
    if (meta->free != 0) {
        RecordBlock* block = (RecordBlock*)getBlock(meta->free);
        if (block == NULL) return NULL;

        meta->free = block->header.next;
        block->header.next = 0;
        block->header.type = BLOCK_TYPE_FREE;
        return block;
    }

    RecordBlock* block = (RecordBlock*)bufferManager.allocateBlock();
    if (block == NULL) return NULL;
    block->header.index = meta->count++;
    block->header.magic = MAGIC_NUM;
    block->header.next = 0;
    block->header.last = 0;

    if (fseek(file, (meta->count - 1)*BLOCK_SIZE, SEEK_SET) != 0 ||
        fwrite(block, BLOCK_SIZE, 1, file) != 1) {
        bufferManager.freeBlock(block);
        return NULL;
    }
    buffers[block->header.index] = block;
    block->header.reserved = 1;

    return block;
}

int StorageManager::freeBlock(uint32_t index) {
    RecordBlock* b = (RecordBlock*)getBlock(index);
    if (b == NULL) return false;
    b->header.next = meta->free;
    meta->free = index;
    return true;
}


uint32_t StorageManager::getIndexOfRoot() {
    return meta->root;
}

void StorageManager::setIndexOfRoot(uint32_t i) {
    //assert(i > 0 && i < meta->count);
    meta->root = i;
}

void RecordManager::nextRoot() {
    RecordBlock* newRoot = (RecordBlock*)s.getFreeBlock();
    newRoot->init();
    newRoot->header.next = root->header.index;
    root->header.last = newRoot->header.index;
    s.setIndexOfRoot(newRoot->header.index);
    root = newRoot;
}

RecordManager::RecordManager(StorageManager & s) :s(s) {
    if (s.getIndexOfRoot() == 0) {
        // uint32_t index = 0;
        root = (RecordBlock*)s.getFreeBlock();
        if (root == NULL) {
            // TODO: error
        }
        s.setIndexOfRoot(root->header.index);
        root->init();
    }
    else {
        root = (RecordBlock*)s.getBlock(s.getIndexOfRoot());
        if (root == NULL) {
            // TODO: error
        }
    }
}

int RecordManager::put(const Record * rcd, Location * loc) {
    loc->index = root->header.index;
    if (!root->addRecord(rcd, &loc->position)) {
        nextRoot();
        int res = root->addRecord(rcd, &loc->position);
        loc->index = root->header.index;
        assert(res == 1);
        return res;
    }
    return 1;
}

int RecordManager::set(const Record * newRcd, Location * loc) {
    RecordBlock *block = (RecordBlock*)s.getBlock(loc->index);
    if (!block->updateRecord(loc->position, newRcd)) {
        block->delRecord(loc->position);
        if (put(newRcd, loc))
            return 2;

        return 0;
    }
    return 1;
}

int RecordManager::del(const Location * loc) {
    RecordBlock *block = (RecordBlock*)s.getBlock(loc->index);
    block->delRecord(loc->position);
    return 1;
}

const Record* RecordManager::get(const Location * loc) {
    RecordBlock *block = (RecordBlock*)s.getBlock(loc->index);
    return block->getRecord(loc->position);
}
