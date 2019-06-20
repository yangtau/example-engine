////
// @file file.h
// @brief
// file
//
// @author yangtao
// @email yangtaojay@gmail.com
//

#pragma once

#include <Windows.h>
#include "block.h"

///
// @brief
// File class
class EXFile {
private:
    HANDLE handle;
public:

    EXFile() : handle(INVALID_HANDLE_VALUE) {}

    ~EXFile() {
        if (handle != INVALID_HANDLE_VALUE)
            CloseHandle(handle);
    }

    // `num` is the number of blocks
    // return 0 if failed to create the file
    // return 2 if the file exists
    // return 1 if a new file is created
    int create(const char* path, uint32_t num);

    // `index` is the order the block in the file
    bool readBlock(uint32_t index, void *block);

    // write `block` to file
    bool writeBlock(uint32_t index, void* block);

    bool resize(uint32_t num);
};
