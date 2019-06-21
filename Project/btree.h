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
#include <vector>
#include "block.h"
#include "storage.h"

#pragma pack(1)

///
// @breif
// node of b+tree
struct NodeBlock {
    BlockHeader header;
    uint16_t count;
    uint8_t keyLen;
    uint8_t valLen;

    // key[1]value
    // there is one bit bettween the key and value to indicate that whether the item is removed

    uint16_t size();

    void init();

    bool full();

    bool empty();

    bool leaf();

    uint16_t find(void* key);

    void insert(void* key, void* value, uint16_t index);

    void split(NodeBlock* nextNode);

    // merge `nextNode` with this
    void merge(NodeBlock* nextNode);

    void remove(uint16_t index);
};

#pragma pack()



typedef int(*Compare)(const void *, const void *);


class BTreeIterator {
    NodeBlock *cur = NULL;
    int index = -1;
    BTree &btree;
    const void *lo;
    const void *hi;

    BTreeIterator(const void *lo, const void *hi, BTree &t);

    int next();

    int forward();

    int set(void *value);

    int get(void *key, void *value);

    // remove by mark
    int remove();
};

struct BTree {
    friend struct BTreeIterator;
    friend struct NodeBlock;

private:
    StorageManager &storage;
    NodeBlock* root;
    uint8_t keyLen;
    uint8_t valLen;

    Compare cmp;

    // range query
    NodeBlock *cur = NULL;
    int indexOfKey = -1; // the index of the current key in the block

public:
    BTree(uint8_t keyLength, uint8_t valueLength, Compare cmp, StorageManager &storage);

    int put(const void* key, const void* value);

    int get(const void* key, void* value);

    int remove(const void *key);

    // range query
    // `lo` or `hi` can be NULL;
    // if `lo` is NULL, it will start from the first items;
    // if `hi` is NULL, it will end in the last item;
    BTreeIterator iterator(const void *lo, const void *hi);
};