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
#include "storage.h"

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
// node of b+tree
struct NodeBlock {
    BlockHeader header;

    // The first key is empty, if the block is an interior node
    KeyValue kv[4];

    static uint16_t size();

    bool isFull();

    bool isEmpty();

    ///
    // @brief
    // the index of first item whose key is bigger than or equal with `key`
    uint16_t find(uint64_t key);

    bool insert(KeyValue item, uint16_t index);

    void split(NodeBlock* nextNode, uint32_t index);
};

#pragma pack()

class BTree {
private:
    StorageManager &storage;
    NodeBlock* root;
    
    //void setRootIndex(uint32_t index);
    KeyValue insert(KeyValue kv, NodeBlock* cur);
    int remove(KeyValue kv, NodeBlock* cur);
    int search(KeyValue kv, NodeBlock* cur);

public:
    BTree(StorageManager &s);
    int insert(KeyValue kv);
    int remove(uint64_t key);
    int search(uint64_t key, uint32_t* value);
};