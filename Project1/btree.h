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

// Block type
#define BLOCK_TYPE_FREE 0
#define BLOCK_TYPE_DATA 1 // data
#define BLOCK_TYPE_META 2 // metadata
#define BLOCK_TYPE_LEAF 3 // leaf node
#define BLOCK_TYPE_NODE 4 // node
#define BLOCK_TYPE_LOG 5 // log

#pragma pack(1)
struct Header
{
	uint16_t type : 3;
	uint16_t reserved : 13;
	uint16_t count;
	uint16_t checksum;
	uint16_t free;
	uint16_t next;
	uint16_t  compute();
};

struct Meta
{
	uint32_t free;
	uint32_t root;
	uint32_t count;
	uint32_t idle;
};

///
// @breif
// kv for B+tree
struct KeyValue
{
	uint64_t key;
	uint32_t value;
};

#pragma pack(0)

struct BTree
{

};