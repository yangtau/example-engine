////
// @file storage.h
// @brief
// storage manager
// 
// @author yangtao
// @email yangtaojay@gmail.com
//

#pragma once

#include "block.h"
#include "buffer.h"
#include <map>


///
// @brief
// storage manager
class StorageManager {
private:
    
    std::map<uint32_t, RecordBlock *> buffers;

    BufferManager bufferManager;

    MetaBlock *meta;

    FILE *file;

    
public:
    explicit StorageManager();

    int create(const char *path);

    int open(const char *path);

    int close();

    ~StorageManager();

    void *getBlock(uint32_t index);

    //const void *readBlock(uint32_t index);

    void* getFreeBlock();

    int freeBlock(uint32_t index);

    bool save();

    uint32_t getIndexOfRoot();

    void setIndexOfRoot(uint32_t);
};