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
#ifdef DEBUG_BTREE
    return 4;
#else
    return ((BLOCK_SIZE - sizeof(BlockHeader)) / sizeof(KeyValue));
#endif // DEBUG_BTREE   
}

bool NodeBlock::full() {
    assert(header.count <= NodeBlock::size());
    return header.count == NodeBlock::size();
}

bool NodeBlock::empty() {
    return header.count == 0;
}

bool NodeBlock::eMin() {
    return header.count == size() / 2;
}

bool NodeBlock::geMin() {
    return header.count >= size() / 2;
}

uint16_t NodeBlock::find(uint64_t key) {
    assert(!empty());
    // TODO: diff between interior node and leaf
    uint16_t i = 0;
    if (!isLeaf()) i++;
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

void NodeBlock::insert(KeyValue item, uint16_t index) {
    assert(index <= header.count);
    memmove(&kv[index + 1], &kv[index],
        sizeof(KeyValue) * (header.count - index));
    kv[index] = item;
    header.count++;
}

void NodeBlock::split(NodeBlock* nextNode) {
    nextNode->header.type = header.type;

    // move kv
    memcpy(&nextNode->kv[0], &kv[(NodeBlock::size() + 1) / 2], NodeBlock::size() / 2 * sizeof(KeyValue));
    this->header.count = (NodeBlock::size() + 1) / 2;
    nextNode->header.count = NodeBlock::size() / 2;

    // chain brother
    nextNode->header.next = this->header.next;
    this->header.next = nextNode->header.index;
}

void NodeBlock::merge(NodeBlock * nextNode) {
    memcpy(&kv[header.count], &nextNode->kv[0], nextNode->header.count * sizeof(KeyValue));

    header.count += nextNode->header.count;

    header.next = nextNode->header.next;
}

bool NodeBlock::isLeaf() {
    assert(header.type == BLOCK_TYPE_LEAF ||
        header.type == BLOCK_TYPE_NODE);
    return header.type == BLOCK_TYPE_LEAF;
}

void NodeBlock::remove(uint16_t index) {
    assert(index < header.count);
    memmove(&kv[index], &kv[index + 1],
        sizeof(KeyValue) * (header.count - index - 1));
    header.count--;
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


// 如果没有分裂，返回的value值为0。否则，返回要插入的值
int BTree::insert(KeyValue kv) {
    if (root->empty()) {
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
        newRoot->header.count = 2;
        newRoot->header.type = BLOCK_TYPE_NODE;
        newRoot->kv[1] = r;
        newRoot->kv[0].value = root->header.index;

        //
        newRoot->kv[0].key = root->kv[0].key;

        root = newRoot;
        storage.setIndexOfRoot(newRoot->header.index);
    }
    return 1;
}

KeyValue BTree::insert(KeyValue kv, NodeBlock* cur) {
    KeyValue res;
    res.value = 0;

    uint16_t i = cur->find(kv.key);

    if (cur->isLeaf()) {
        // no repetition key so far
        if (kv.key != cur->kv[i].key)
            cur->insert(kv, i);
        else
            cur->kv[i] = kv;
    }
    else {
        if (cur->kv[i].key > kv.key || i == cur->header.count) {
            i--;
        }
        NodeBlock* t = (NodeBlock*)storage.getBlock(cur->kv[i].value);
        KeyValue r = insert(kv, t);
        if (r.value != 0)
            cur->insert(r, cur->find(r.key));

        //
        if (i == 0)
            cur->kv[i].key = t->kv[0].key;
    }

    if (cur->full()) {
        uint32_t index = 0;

        NodeBlock* nextNode = (NodeBlock*)storage.getFreeBlock(&index);
        if (nextNode == NULL) {
            // TODO: failed to allocate a new block
        }
        cur->split(nextNode);
        res = nextNode->kv[0];
        res.value = index;
        return res;
    }
    return res;
}

void BTree::remove(uint64_t key, NodeBlock* cur) {
    uint16_t i = cur->find(key);

    if (cur->isLeaf()) {
        if (cur->kv[i].key > key) return;
        else {
            cur->remove(i);
        }
    }
    else {
        if (cur->kv[i].key > key || i == cur->header.count) {
            i--;
        }
        NodeBlock* s = (NodeBlock*)storage.getBlock(cur->kv[i].value);
        remove(key, s);

        // update the key in parent
        cur->kv[i].key = s->kv[0].key;

        if (s->geMin()) return;

        // maintain
        if (i > 0) {
            NodeBlock *left = (NodeBlock*)storage.getBlock(cur->kv[i - 1].value);
            if (left->eMin()) {
                // merge
                left->merge(s);
                cur->remove(i);

                storage.freeBlock(s->header.index);
            }
            else {
                s->insert(left->kv[left->header.count - 1], 0);
                left->remove(left->header.count - 1);
            }
        }
        else {
            NodeBlock *right = (NodeBlock*)storage.getBlock(cur->kv[i + 1].value);
            if (right->eMin()) {
                // merge
                s->merge(right);
                cur->remove(i + 1);

                storage.freeBlock(right->header.index);
            }
            else {
                s->insert(right->kv[0], s->header.count);
                right->remove(0);
                cur->kv[i + 1].key = right->kv[0].key;
            }
        }
    }
}


int BTree::remove(uint64_t key) {
    if (root->empty()) {
        return 0;
    }
    remove(key, root);
    if (!root->isLeaf() && root->header.count == 1) {
        NodeBlock* s = (NodeBlock*)storage.getBlock(root->kv[0].value);
        storage.setIndexOfRoot(s->header.index);
        storage.freeBlock(root->header.index);
        root = s;
    }
    return 1;
}


KeyValue BTree::search(uint64_t key, NodeBlock* cur) {
    uint16_t i = cur->find(key);
    if (cur->isLeaf()) {
        if (cur->kv[i].key == key)
            return cur->kv[i];
    }
    else {
        if (cur->kv[i].key > key) i--;
        NodeBlock* s = (NodeBlock*)storage.getBlock(root->kv[i].value);
        return search(key, s);
    }
    KeyValue res;
    res.value = 0;
    return res;
}


KeyValue BTree::search(uint64_t key) {
    return search(key, root);
}
