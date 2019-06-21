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
//#include <vector>
#include <assert.h>
#include "block.h"
#include "storage.h"


typedef int(*Compare)(const void *, const void *);

#pragma pack(1)

struct NodeBlock {
    BlockHeader header;
    uint16_t count;
    uint8_t kv[0]; // key[1]value
    void init(uint16_t type) {
        count = 0; header.next = 0;
        header.last = 0; header.type = type;
    }
};


class BTree;
///
// @breif
// node of b+tree
struct BTreeNode {
    uint8_t keylen;
    uint8_t vallen = 0;
    NodeBlock *block;
    BTree &btree;

    BTreeNode(BTree &btree, void *block) :
        btree(btree),
        keylen(btree.keylen),
        block((NodeBlock*)block) {
        assert(this->block->header.type == BLOCK_TYPE_LEAF
            || this->block->header.type == BLOCK_TYPE_NODE);
        if (this->block->header.type == BLOCK_TYPE_LEAF) vallen = btree.vallen;
        else if (this->block->header.type == BLOCK_TYPE_NODE) vallen = sizeof(uint32_t);
    }

    ~BTreeNode() {}

    uint16_t sizeofkv() const {
        return keylen + vallen + 1; // 1bytes for flag
    }

    uint16_t size() const {
        return (BLOCK_SIZE - sizeof(BlockHeader) - sizeof(uint16_t)) / sizeofkv();
    }

    int count() const { return block->count; }

    bool full() const {
        assert(block->count <= size());
        return block->count == size();
    };

    bool empty() const {
        return block->count == 0;
    }

    bool leaf() const {
        return block->header.type == BLOCK_TYPE_LEAF;
    }

    int next();

    int last();

    // return -1 if no such key exists
    int find(const void* key) const;

    // least upper bound
    // return `count` if `key` is bigger than all items in the block
    int lub(const void *key) const;

    const void* maxKey() const;

    // value in kv[index]
    const void* getValue(int index) const;

    const void* getKey(int index) const;

    void setKey(const void* key, uint16_t index);

    void setValue(const void* value, uint16_t index);

    void insert(const void* key, const void* value, uint16_t index);


    void split(NodeBlock* nextNode);

    // merge `nextNode` with this block
    void merge(NodeBlock* nextNode);

    void remove(uint16_t index);
};

#pragma pack()


class BTreeIterator {
    BTreeNode *cur = NULL;
    int index = -1;
    BTree &btree;
    void *lo = NULL;
    void *hi = NULL;


    BTreeIterator(BTree &bt) : btree(bt) {}

    ~BTreeIterator() {
        if (lo != NULL) free(lo);
        if (hi != NULL) free(hi);
        if (cur != NULL) delete cur;
    }
    int open(const void *lo, const void *hi);

    void close();

    // return 0 if it ends
    int next();

    int forward();

    void set(void *value);

    void get(void *key, void *value) const;

    // remove by flag
    void remove();
};



class BTree {
    friend class BTreeIterator;
    friend class BTreeNode;

private:
    StorageManager &storage;
    BTreeNode* root = NULL;
    const uint8_t keylen;
    const uint8_t vallen;

    const Compare cmp;

    BTreeNode* getLeaf(const void *key);

    BTreeNode* getNode(uint32_t index);

    int insert(const void* key, const void* value, BTreeNode* cur);

public:
    BTree(uint8_t keylen, uint8_t vallen, Compare cmp, StorageManager &storage);

    ~BTree() {
        if (root != NULL) delete root;
    }

    int put(const void* key, const void* value);

    int get(const void* key, void* value);

    int remove(const void *key);


    // range query
    // `lo` or `hi` can be NULL;
    // if `lo` is NULL, it will start from the first items;
    // if `hi` is NULL, it will end in the last item;

    BTreeIterator iterator();
};
