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
    StorageManager();
    StorageManager(const char *path);

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

class RecordManager {
private:
    StorageManager &s;
    RecordBlock *root;
    void nextRoot();
public:
    struct Location {
        uint32_t index; // index of block
        uint16_t position; // position in block
    };
    RecordManager(StorageManager &s);
    int put(const Record*rcd, Location*loc);

    // if loc is updated, return 2
    int set(const Record* newRcd, Location *loc);
    int del(const Location *loc);
    const Record* get(const Location *loc);
};