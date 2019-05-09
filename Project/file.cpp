////
// @file file.cpp
// @brief
// block
//
// @author yangtao
// @email yangtaojay@gmail.com
//

#include "file.h"

int File::initFile(uint32_t num) {
    // init metadata block
    meta = (MetaBlock*)readBlock(META_INDEX);
    meta->header.type = BLOCK_TYPE_META;
    meta->header.next = 0;
    meta->root = 0;
    meta->free = META_INDEX + 1;
    meta->count = num;
    meta->free = 0;
    meta->idle = num - 1;

    // free block list
    for (uint32_t i = META_INDEX + 1; i < num; i++) {
        RecordBlock *block = (RecordBlock*)readBlock(i);
        if (block == NULL) return false;

        block->header.type = BLOCK_TYPE_FREE;
        // add to free lsit
        block->header.next = meta->free;
        meta->free = i;

        if (!writeBlock(i, block)) return false;
    }
    return true;
}

void * File::readBlock(uint32_t index) {
    BufferBlock * buffer = (BufferBlock *)bufferManager.allocBlock();
    if (buffer == NULL) {
        // TODO: error handle
        return NULL;
    }

    // offset
    uint64_t offset = index * BLOCK_SIZE;  // the position of the block
    if (SetFilePointer(handle, offset, NULL, FILE_BEGIN) != offset)
        return NULL;

    // read
    unsigned long bytesToRead = -1;
    if (!ReadFile(handle, buffer, BLOCK_SIZE, &bytesToRead, NULL))
        return NULL;

    return buffer;
}

int File::create(const char* path, uint32_t num) {
    // open file
    handle = CreateFileA((LPCSTR)path, GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        FILE_FLAG_NO_BUFFERING, nullptr);
    // read metadata block
    if (handle != INVALID_HANDLE_VALUE)
        meta = (MetaBlock*)readBlock(META_INDEX);


    if (handle == INVALID_HANDLE_VALUE) {
        // the file does not exist
        // create file
        handle = CreateFileA((TCHAR*)path, GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_DELETE, nullptr, CREATE_ALWAYS,
            FILE_FLAG_NO_BUFFERING, nullptr);
        if (handle == INVALID_HANDLE_VALUE)
            return false;

        // set the size of file, `BLOCK_SIZE * num` is the size of file
        BOOL res = SetFilePointer(handle, BLOCK_SIZE * num, NULL, FILE_BEGIN) &&
            SetEndOfFile(handle);
        if (!res)
            return false;

        // initialize file
        if (!initFile(num)) return false;
    }
    return true;
}

void* File::getBlock(uint32_t index) {
    if (meta == NULL || index >= meta->count) return NULL;

    RecordBlock* block = (RecordBlock*)readBlock(index);

    if (!(block->header.check())) {
        // TODO: wrong checksum
    }
    return block;
}

void * File::getFreeBlock(uint32_t * index) {
    // TODO: alloc more block
    if (meta->idle || meta->free == 0)
        return NULL;

    RecordBlock *b = (RecordBlock*)getBlock(meta->free);
    if (b == NULL) return NULL;

    // remove from free list
    meta->free = b->header.next;

    meta->idle--;

    return b;
}

int File::writeBlock(uint32_t index, void* _block) {
    RecordBlock *block = (RecordBlock *)_block;
    // offset
    uint64_t offset = index * BLOCK_SIZE;  // the position of the block

    if (SetFilePointer(handle, offset, NULL, FILE_BEGIN) != offset)
        return false;

    // compute checksum
    block->header.compute();

    // write
    unsigned long bytesToWrite = -1;
    if (!WriteFile(handle, block, BLOCK_SIZE, &bytesToWrite, NULL) &&
        bytesToWrite == BLOCK_SIZE)
        return false;
    // free block buffer
    bufferManager.freeBlock(block);
    return true;
}
