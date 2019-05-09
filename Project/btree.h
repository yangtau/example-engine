////
// @file btree.h
// @breif
// B+tree
//
// @author yangtao
// @email yangtaojay@gamil.com
//  
#pragma once
#include <inttypes.h>
#include "block.h"

#pragma pack(1)

///
// @breif
// kv for B+tree
struct KeyValue {
    uint64_t key;
    uint32_t value;
};

///
// @breif
// interior node and root of b+tree
struct BTreeNodeBlock {
    BlockHeader header;
    KeyValue kv[1];
};

///
// @brief 
// leaf of b+tree
struct BTreeLeafBlock {
    BlockHeader header;
};

#pragma pack(0)