////
// @file btree.h
// @breif
// B+tree
//
// @author yangtao
// @email yangtaojay@gmail.com
//

#ifndef _BTREE_BTREE_H
#define _BTREE_BTREE_H

#include <assert.h>
#include <inttypes.h>
#include "block.h"
#include "buffer.h"

typedef int(Compare)(const void*, const void*, void*);

class BTree;

#pragma pack(1)

/*
 * @brief
 * node of b+tree
 */
struct NodeBlock {
    BlockHeader header;
    u16 count;
    u16 keySize;
    u16 valSize;
    BTree* btree;
    u8 kv[0];  // key[1]value

    void init(u32 index, u16 type, u16 keySize, u16 valSize, BTree* b = NULL) {
        header.init(index);
        count = 0;
        header.type = type;
        this->keySize = keySize;
        this->valSize = valSize;
        btree = b;
    }

    void setBTree(BTree* t) { btree = t; }

    u16 sizeOfItem() const {
        return keySize + valSize + 1;  // 1bytes for flag
    }

    u16 size() const;

    bool full() const {
        assert(count <= size());
        return count == size();
    };

    bool empty() const { return count == 0; }

    bool leaf() const { return header.type == BLOCK_TYPE_LEAF; }

    // return -1 if no such key exists
    int find(const void* key) const;

    // least upper bound
    // return `count` if `key` is bigger than all items in the block
    int lub(const void* key) const;

    const void* maxKey() const;

    // value in kv[index]
    const void* getValue(int index) const;

    const void* getKey(int index) const;

    // insert kv in index, the items after index will be move backward
    void insert(const void* key, const void* value, u16 index);

    void insert(const void* key, const void* value) {
        insert(key, value, lub(key));
    }

    // if key or value is null, it will remain the origin key or value;
    // default flag is `1`
    void set(u16 index, const void* key, const void* value, u8 flag = 1);

    void split(NodeBlock* nextNode);

    // merge `nextNode` with this block
    void merge(NodeBlock* nextNode);

    void remove(u16 index);

    bool removed(u16 index) const;

    void removeByFlag(u16 index);

    int compare(u16 index, const void* key) const;
};

struct BTreeMetadata {
    BlockHeader header;
    u32 root;  // index of root
    u32 countOfItem;
    u32 countOfBlock;   // the number of used blocks
    u32 numberOfBlock;  // the number of all blocks

    void init(u32 index) {
        header.init(index);
        header.type = BLOCK_TYPE_META;
        root = 0;
        countOfItem = 0;
        countOfBlock = 0;
        numberOfBlock = 0;
    }
};

#pragma pack()

class BTree {
    friend struct NodeBlock;

public:
    // location of value in data block
#pragma pack(1)
    struct Location {
        u32 index;     // index of block
        u16 position;  // position in block
    };
#pragma pack(0)

    enum IterFlag {
        EXACT_KEY = 0,
        KEY_OR_NEXT = 1,
        KEY_OR_PREV = 2,
        AFTER_KEY = 3,
        BEFROE_KEY = 4
    };

    class Iterator {
        NodeBlock* cur = NULL;
        int index = -1;
        BTree* btree = NULL;

    public:
        Iterator(BTree* b) : btree(b) {}
        Iterator() {}
        void setBTree(BTree* b) { btree = b; }

        ~Iterator() {}

        // @param flag   0 exact the key
        //               1 the key or next
        //               2 the key or prev
        //               3 after the key
        //               4 before the key
        int locate(const void* key, IterFlag flag);

        int first();

        int last();

        // return 0 if failed to move to the next key
        int next();

        // return 0 if failed to move to the previous key
        int prev();

        const void* getKey();

        const void* getValue();

        // compare current key with `key`
        int compare(const void* key) const;

        // remove by flag
        void remove();
    };

private:
    NodeBlock* root = NULL;
    BTreeMetadata* meta = NULL;

    const u16 keySize;
    typedef u32 NodeValue;
    const u16 nodeValueSize = sizeof(NodeValue);

    typedef Location LeafValue;  // for nonconst value

    const u16 leafValueSize;
    const bool isConstValueSize;

    Compare* cmp;
    void* extraCmpInfo;  // extra information used in comparison

    // buffer & file
    BufferManager* buffer = NULL;
    MyFile file;
    const static u32 blockSize = 4096;

    NodeBlock* getLeaf(const void* key);

    NodeBlock* getFirstLeaf();

    NodeBlock* getLastLeaf();

    NodeBlock* getNodeBlock(u32 index);

    NodeBlock* getFreeNodeBlock(u16 type, u16 keySize, u16 valueSize);

    DataBlock* getFreeDataBlock();

    int insert(const void* key,
        const void* value,
        u32 valueSize,
        NodeBlock* cur);

    int insertValue(u32 index, const void* value, u32 valueSize, Location* loc);

    const void* getValue(Location loc);

public:
    BTree(u16 keySize,
        Compare* c,
        void* extraCmpInfo = NULL,
        u16 valueSize = 0xffff);

    int create(const char* filename);

    int open(const char* filename);

    int save() {
        int res = buffer->save();
        meta->header.reserved = 1;
        root->header.reserved = 1;
        return res;
    }

    ~BTree() {
        if (buffer && !buffer->save()) {
            fprintf(stderr, "buffer save");
        }
        delete buffer;
        buffer = NULL;
    }

    u32 countOfItems() { return meta->countOfItem; }

    void setCmp(Compare cmp) { this->cmp = cmp; }

    // if the size of value is constant, `valueSize` should be -1
    int put(const void* key, const void* value, u32 valueSize = -1);

    //    int get(const void *key, void *value);

    //    int del(const void *key);

    Iterator iterator() { return Iterator(this); };
};

#endif  //_BTREE_BTREE_H