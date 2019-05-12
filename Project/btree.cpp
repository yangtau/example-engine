////
// @file btree.cpp
// @brief
// B+tree
//
// @author yangtao
// @email yangtaojay@gmail.com
//

#include "btree.h"
#include <assert.h>
#include <cstring>

uint16_t NodeBlock::size() {
    // keep an empty item as temp used when splitting a full block
    return 4;
    //return ((BLOCK_SIZE - sizeof(BlockHeader)) / sizeof(KeyValue));
}

bool NodeBlock::isFull() {
    assert(header.count <= NodeBlock::size());
    return header.count == NodeBlock::size();
}

bool NodeBlock::isEmpty() {
    return header.count == 0;
}

uint16_t NodeBlock::find(uint64_t key) {
    assert(!isEmpty());
    // TODO: diff between interior node and leaf
    uint16_t i = 0;
    if (header.type == BLOCK_TYPE_NODE) i++;
    uint16_t j = header.count - 1;
    if (kv[j].key < key) return header.count;
    while (i < j) {
        uint16_t mid = (i + j) / 2;
        if (kv[mid].key < key)
            i = mid + 1;
        else
            j = mid;
    }
    return j;
}

bool NodeBlock::insert(KeyValue item, uint16_t index) {
    assert(index <= header.count);
    memmove(&kv[index + 1], &kv[index],
        sizeof(KeyValue) * (header.count - index));
    kv[index] = item;
    header.count++;
    return true;
}

void NodeBlock::split(NodeBlock* nextNode, uint32_t index) {
    nextNode->header.type = header.type;

    // move kv
    memcpy(&nextNode->kv[0], &kv[(NodeBlock::size() + 1) / 2], NodeBlock::size() / 2 * sizeof(KeyValue));
    this->header.count = (NodeBlock::size() + 1) / 2;
    nextNode->header.count = NodeBlock::size() / 2;

    if (nextNode->header.type == BLOCK_TYPE_LEAF) {
        nextNode->header.next = this->header.next;
        this->header.next = index;
    }
}

//void BTree::setRootIndex(uint32_t index) {
//    rootIndex = index;
//    storage.setIndexOfRoot(rootIndex);
//}

// 如果没有分裂，返回的value值为0。否则，返回要插入的值
KeyValue BTree::insert(KeyValue kv, NodeBlock* cur) {
    assert(cur->header.type == BLOCK_TYPE_LEAF ||
        cur->header.type == BLOCK_TYPE_NODE);

    KeyValue res;
    res.value = 0;

    uint16_t i = cur->find(kv.key);

    if (cur->header.type == BLOCK_TYPE_LEAF) {
        cur->insert(kv, i);
    }
    else {
        if (cur->kv[i].key > kv.key) {
            i--;
        }
        NodeBlock* t = (NodeBlock*)storage.getBlock(cur->kv[i].value);
        KeyValue r = insert(kv, t);
        if (r.value != 0)
            cur->insert(r, cur->find(r.key));
    }
    if (cur->isFull()) {
        uint32_t index = 0;

        NodeBlock* nextNode = (NodeBlock*)storage.getFreeBlock(&index);
        if (nextNode == NULL) {
            // TODO: failed to allocate a new block
        }
        cur->split(nextNode, index);
        res = nextNode->kv[0];
        res.value = index;
        return res;
    }
    return res;
}

int BTree::remove(KeyValue kv, NodeBlock* cur) {
    return 0;
}

int BTree::search(KeyValue kv, NodeBlock* cur) {
    return 0;
}

BTree::BTree(StorageManager &s) :root(NULL), storage(s) {
    if (storage.getIndexOfRoot() == 0) {
        uint32_t index = 0;
        root = (NodeBlock*)storage.getFreeBlock(&index);
        if (root == NULL) {
            // TODO: 
        }
        storage.setIndexOfRoot(index);
        root->header.count = 0;
        root->header.type = BLOCK_TYPE_LEAF;
    }
    else {
        root = (NodeBlock*)storage.getBlock(storage.getIndexOfRoot());
        if (root == NULL) {
            // TODO: 
        }
    }
}


int BTree::insert(KeyValue kv) {
    if (root->isEmpty()) {
        root->insert(kv, 0);
        return 1;
    }
    KeyValue r = insert(kv, root);
    if (r.value != 0) {
        uint32_t index = 0;
        NodeBlock* newRoot = (NodeBlock*)storage.getFreeBlock(&index);
        if (newRoot == NULL) {
            // TODO: failed to allocate a new block
        }
        newRoot->header.count = 1;
        newRoot->header.type = BLOCK_TYPE_NODE;
        newRoot->kv[1] = r;
        newRoot->kv[0].value = root->header.index;

        root = newRoot;
        storage.setIndexOfRoot(newRoot->header.index);
    }
    return 1;
}

int BTree::remove(uint64_t key) {
    return 0;
}

int BTree::search(uint64_t key, uint32_t* value) {
    return 0;
}
