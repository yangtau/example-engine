////
// @file block.h
// @brief
// block
//
// @author yangtao
// @email yangtaojay@gmail.com
//

#pragma once
#include <inttypes.h>
// Block type
#define BLOCK_TYPE_FREE 0  // free block
#define BLOCK_TYPE_DATA 1  // data block
#define BLOCK_TYPE_META 2  // metadata block
#define BLOCK_TYPE_LEAF 3  // leaf  block
#define BLOCK_TYPE_NODE 4  // interior node block
#define BLOCK_TYPE_LOG 5   // log block

#pragma pack(1)

struct RecordHeader {
    uint16_t size;       // bytes of data
    uint16_t timestamp;  // TODO: size of timestamp
};

struct Record {
    RecordHeader header;
    uint8_t* getData();
};

/// 
// @brief
// header of block
// 96
struct BlockHeader {
    uint16_t type : 3;  // 块类型
    uint16_t reserved : 13;
    uint16_t checksum;  // Tcp头部校验
    uint16_t count;     // 记录数目
    uint16_t free;      // 空闲块
    uint32_t next;      // 下一个block的文件偏移量

    uint16_t compute();  // compute checksum and set it
    int check();         // check checksum
};

struct Tailer {
    uint16_t slots[1];  // 向上增长的记录偏移量数组
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

    int addRecord(Record* record);

    int delRecord(uint32_t position);

    Record* getRecord(uint32_t position);

    int updateRecord(uint32_t position, Record* record);
};

///
// @brief
// metadata blcok
struct MetaBlock {
    BlockHeader header;
    uint32_t free; // list of free block
    uint32_t root; // root of b+tree
    uint32_t count; // num of blocks
    uint32_t idle; // num of idle blocks
};