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

typedef int(Compare)(const void *, const void *, void *);

#pragma pack(1)

///
// @breif
// node of b+tree
struct NodeBlock {
  BlockHeader header;
  uint16_t count;
  uint8_t keylen;
  uint8_t vallen;
  Compare *cmp;
  void *cmpp;
  uint8_t kv[0];  // key[1]value

  void init(uint16_t type, uint8_t keylen, uint8_t vallen) {
    count = 0;
    header.next = 0;
    header.last = 0;
    header.type = type;
    this->keylen = keylen;
    this->vallen = vallen;
  }

  void setCmp(Compare *c, void *p) {
    cmp = c;
    cmpp = p;
  }

  uint16_t sizeofkv() const {
    return keylen + vallen + 1;  // 1bytes for flag
  }

  uint16_t size() const;

  bool full() const {
    assert(count <= size());
    return count == size();
  };

  bool empty() const { return count == 0; }

  bool leaf() const { return header.type == BLOCK_TYPE_LEAF; }

  // return -1 if no such key exists
  int find(const void *key) const;

  // least upper bound
  // return `count` if `key` is bigger than all items in the block
  int lub(const void *key) const;

  const void *maxKey() const;

  // value in kv[index]
  const void *getValue(int index) const;

  const void *getKey(int index) const;

  // insert kv in index, the items after index will be move backward
  void insert(const void *key, const void *value, uint16_t index);

  void insert(const void *key, const void *value) {
    insert(key, value, lub(key));
  }

  // if key or value is null, it will remain the origin key or value;
  // default flag is `1`
  void set(uint16_t index, const void *key, const void *value,
           uint8_t flag = 1);

  void split(NodeBlock *nextNode);

  // merge `nextNode` with this block
  void merge(NodeBlock *nextNode);

  void remove(uint16_t index);

  bool removed(uint16_t index) const;

  void removeByFlag(uint16_t index);

  // TODO: compare with key
};

#pragma pack()

class BTree {
  friend class BTreeIterator;

 public:
  class Iterator {
    NodeBlock *cur = NULL;
    int index = -1;
    BTree &btree;
    void *lo = NULL;
    void *hi = NULL;
    // bool status = false;
    bool opened = false;

   public:
    Iterator(BTree &bt) : btree(bt) {}

    ~Iterator() { close(); }

    // [lo, hi)
    // after iter opened, it will pointe at `lo` or item bigger than `lo`
    // lo and hi can be null
    // if inverse is 0, place the first postion at hi
    int open(const void *lo, const void *hi, int inverse = 0);

    void close();

    // return 0 if it ends
    int next();

    int prev();

    void set(void *value);

    void get(void *key, void *value) const;

    const void *getKey() const;

    // remove by flag
    void remove();
  };

 private:
  StorageManager &storage;
  NodeBlock *root = NULL;
  const uint8_t keylen;
  const uint8_t vallen;

  Compare *cmp;
  void *cmpp;

  NodeBlock *getLeaf(const void *key);

  NodeBlock *getFirstLeaf();

  NodeBlock *getBlock(uint32_t index);

  NodeBlock *getFreeBlock();

  int insert(const void *key, const void *value, NodeBlock *cur);

 public:
  BTree(uint8_t keylen, uint8_t vallen, StorageManager &storage, Compare *c,
        void *p = NULL);

  ~BTree() {}

  uint32_t size() {
    if (root->leaf()) return root->count;
    return root->count * root->size();
  }

  int put(const void *key, const void *value);

  int get(const void *key, void *value);

  int remove(const void *key);

  // range query
  // `lo` or `hi` can be NULL;
  // if `lo` is NULL, it will start from the first items;
  // if `hi` is NULL, it will end in the last item;
  Iterator *iterator();
};
