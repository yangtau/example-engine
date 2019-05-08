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

#pragma pack(1)

struct RecordHeader {
    uint16_t size;       // only bytes of data
    uint16_t timestamp;  // TODO: size of timestamp
};

struct Record {
    RecordHeader header;
    uint8_t* getData();  // what the record keep
};

struct BlockHeader {
    uint16_t type;      // 块类型
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

struct Block {
    BlockHeader header;  

    Tailer* getTailer();

    int addRecord(Record* record);

    int delRecord(uint32_t position);

    Record* getRecord(uint32_t position);

    int updateRecord(uint32_t position, Record* record);
};
