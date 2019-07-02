////
// @file block.h
// @brief
// block
//
// @author yangtao
// @email yangtaojay@gmail.com

#ifndef _BTREE_BLOCK_H
#define _BTREE_BLOCK_H

#include "common.h"
#include <inttypes.h>
// Block type
#define BLOCK_TYPE_FREE 0  // free block
#define BLOCK_TYPE_DATA 1  // data block
#define BLOCK_TYPE_META 2  // metadata block
#define BLOCK_TYPE_LEAF 3  // leaf  block
#define BLOCK_TYPE_NODE 4  // interior node block or root block
#define BLOCK_TYPE_LOG 5   // log block

#define MAGIC_NUM 0x1ef0c6c1

const u32 BLOCK_SIZE = 4096;

#pragma pack(1)

///
// @brief
// header of block
// 12
struct BlockHeader {
    u32 magic;
    u16 type : 3;  // type of block
    u16 reserved : 13;
    u16 checksum;
    u32 index;  // index of this block
    u32 next;   // index of next block
    u32 last;

    u16 compute();  // compute checksum and set it
    int check();    // check checksum
    void init(u32 index) {
        this->index = index;
        magic = MAGIC_NUM;
        //reserved = 0;
        next = 0;
        last = 0;
    }
};
//
//struct Record {
//    u16 size;  // size of the record
//    u8 data[0];
//};

///
// @brief
// record block
// 12 + 8
struct DataBlock {
    BlockHeader header;
    u16 count;  // the number of records in this block
    u16 free;   //

    u16 directory[0];

    void init(u32 index);

    int freeSize();

    int insert(const void *data, u16 size, u16 *position);

//    int del(u16 position);

    const void *get(u16 position);

//    int update(u16 position, const Record *record);
};

#pragma pack()

#endif //_BTREE_BLOCK_H