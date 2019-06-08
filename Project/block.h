////
// @file block.h
// @brief
// block
//
// @author yangtao
// @email yangtaojay@gmail.com

#pragma once
#include <inttypes.h>
// Block type
#define BLOCK_TYPE_FREE 0  // free block
#define BLOCK_TYPE_DATA 1  // data block
#define BLOCK_TYPE_META 2  // metadata block
#define BLOCK_TYPE_LEAF 3  // leaf  block
#define BLOCK_TYPE_NODE 4  // interior node block or root block
#define BLOCK_TYPE_LOG 5   // log block

#define MAGIC_NUM 0x1ef0c6c1

#pragma pack(1)

struct RecordHeader {
    uint16_t size;       // bytes of data
    uint16_t timestamp;  // TODO: size of timestamp
};

struct Record {
    RecordHeader header;
    uint8_t data[1];
};

///
// @brief
// header of block
// 96
struct BlockHeader {
    uint32_t magic;
    uint16_t type : 3;  // type of block
    uint16_t reserved : 13;
    uint16_t checksum;
    uint16_t count;
    uint16_t free;
    uint32_t next;   // index of next block
    uint32_t index;  // index of this block

    uint16_t compute();  // compute checksum and set it
    int check();         // check checksum
};

struct Tailer {
    uint16_t slots[1];
};

#pragma pack()

const uint32_t BLOCK_SIZE = 4096;

///
// @brief
// block of record
struct RecordBlock {
    BlockHeader header;

    Tailer* getTailer();

    void init();

    int addRecord(Record* record, uint32_t* position);

    int delRecord(uint32_t position);

    Record* getRecord(uint32_t position);

    int updateRecord(uint32_t position, Record* record);

    bool full();
};

///
// @brief
// metadata blcok
struct MetaBlock {
    BlockHeader header;
    uint32_t free;      // the index of first free block
    uint32_t root;      // root of b+tree
    uint32_t count;     // num of blocks
    uint32_t freeList;  // list of free block, 0 indicate no free block
};