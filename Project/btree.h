////
// @file btree.h
// @breif
// B+tree
//
// @author yangtao
// @email yangtaojay@gamil.com
//

#pragma once
#define DEBUG_BTREE

#include <inttypes.h>
#include <vector>
#include "block.h"
#include "storage.h"

#pragma pack(1)

///
// @breif
// kv for B+tree
struct KeyValue {
    uint64_t key;
    uint32_t value;
    uint32_t position;
};

///
// @breif
// node of b+tree
struct NodeBlock {
    BlockHeader header;

    // The first key is empty, if the block is an interior node
#ifdef DEBUG_BTREE
    KeyValue kv[4];
#else
    KeyValue kv[1];
#endif // DEBUG_BTREE   

    static uint16_t size();

    bool full();

    bool empty();

    // equal
    bool eMin();

    // greater or equal
    // no need to maintain
    bool geMin();

    ///
    // @brief
    // return the index of first item whose key is not less than `key`
    // if keys of all items is less than `key`, return `header.count`
    uint16_t find(uint64_t key);

    void insert(KeyValue item, uint16_t index);

    void split(NodeBlock* nextNode);

    void merge(NodeBlock* nextNode);

    bool isLeaf();

    void remove(uint16_t index);
};

#pragma pack()

class BTree {
private:
    StorageManager &storage;
    NodeBlock* root;

    //void setRootIndex(uint32_t index);
    KeyValue insert(KeyValue kv, NodeBlock* cur);
    void remove(uint64_t key, NodeBlock* cur);
    void removeByMark(uint64_t key, NodeBlock* cur);
    KeyValue search(uint64_t key, NodeBlock* cur);

public:
    BTree(StorageManager &s);
    int insert(KeyValue kv);
    int remove(uint64_t key);
    KeyValue search(uint64_t key);
    std::vector<KeyValue> search(uint64_t lo, uint64_t hi);
    std::vector<KeyValue> lower(uint64_t lo);
    std::vector<KeyValue> upper(uint64_t hi);
};