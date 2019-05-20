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
#include "file.h"
#include "buffer.h"
#include <map>


///
// @brief
// storage manager
class StorageManager {
private:
    // `header.reserved` is used as a flag indicating whether the buffer is clean
    std::map<uint32_t, RecordBlock *> buffers;

    File file;

    BufferManager bufferManager;

    MetaBlock *meta;

    bool initFile();
public:
    StorageManager(const char *path);

    ~StorageManager();

    void *getBlock(uint32_t index);

    const void *readBlock(uint32_t index);

    void* getFreeBlock(uint32_t *index);

    void freeBlock(uint32_t index);

    bool save();

    uint32_t getIndexOfRoot();

    void setIndexOfRoot(uint32_t);
};