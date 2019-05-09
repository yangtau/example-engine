////
// @file file.h
// @brief
// file
//
// @author yangtao
// @email yangtaojay@gmail.com
//

#pragma once

#include "Windows.h"
#include "block.h"
#include "buffer.h"

///
// @brief
// File class
class File {
private:
    const static  uint32_t META_INDEX = 0;
    HANDLE handle;
    BufferManager bufferManager;
    MetaBlock *meta;
    // `num` is the number of blocks
    int initFile(uint32_t num);
    void *readBlock(uint32_t index);
public:
    File() : handle(INVALID_HANDLE_VALUE), meta(NULL) {}

    ~File() {
        if (meta != NULL)
            if (!writeBlock(META_INDEX, meta)) {
                //TODO: error handle
            }
        if (handle != INVALID_HANDLE_VALUE)
            CloseHandle(handle);
    }

    // `num` is the number of blocks
    int create(const char* path, uint32_t num);

    // `index` is the order the block in the file
    void* getBlock(uint32_t index);

    // get a free block, write the index of block in `index`
    void *getFreeBlock(uint32_t *index);

    // write `block` to file
    // `block` will be free after writing
    int writeBlock(uint32_t index, void* block);
};
