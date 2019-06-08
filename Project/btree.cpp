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

bool NodeBlock::eqMin() {
    return header.count == size() / 2;
}

bool NodeBlock::geMin() {
    return header.count >= size() / 2;
}

uint16_t NodeBlock::find(uint64_t key) {
    assert(!empty());
    uint16_t i = 0;
    //    if (!isLeaf()) i++;
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

void NodeBlock::split(NodeBlock *nextNode) {
    nextNode->header.type = header.type;

    // move kv
    memcpy(&nextNode->kv[0], &kv[(NodeBlock::size() + 1) / 2], NodeBlock::size() / 2 * sizeof(KeyValue));
    this->header.count = (NodeBlock::size() + 1) / 2;
    nextNode->header.count = NodeBlock::size() / 2;

    // chain brother
    nextNode->header.next = this->header.next;
    this->header.next = nextNode->header.index;
}

void NodeBlock::merge(NodeBlock *nextNode) {
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
    memmove(&kv[index], &kv[index + 1], sizeof(KeyValue) * (header.count - index - 1));
    header.count--;
}

BTree::BTree(StorageManager &s) : root(NULL), storage(s) {
    if (storage.getIndexOfRoot() == 0) {
        // uint32_t index = 0;
        root = (NodeBlock *)storage.getFreeBlock();
        if (root == NULL) {
            // TODO: 
        }
        storage.setIndexOfRoot(root->header.index);
        root->header.count = 0;
        root->header.type = BLOCK_TYPE_LEAF;
    }
    else {
        root = (NodeBlock *)storage.getBlock(storage.getIndexOfRoot());
        if (root == NULL) {
            // TODO: 
        }
    }
}

/*
 * b+tree 高度变化在这个函数中发生
 * bool insert(KeyValue, NodeBlock*) 只会发生子节点的分裂, 不会使得bTree变高
 * 如果插入完成后, root节点变满, 分裂root, 并且生成新的root
 */
int BTree::insert(KeyValue kv) {
    if (root->empty()) {
        root->insert(kv, 0);
        return 1;
    }
    if (!insert(kv, root)) return false;

    // root is full
    if (root->full()) {
        NodeBlock *newRoot = (NodeBlock *)storage.getFreeBlock();// new root
        NodeBlock *nextNode = (NodeBlock *)storage.getFreeBlock();// next node of old root
        if (newRoot == NULL || nextNode == NULL) {
            // TODO: error
            return false;
        }
        // init of new root
        newRoot->header.count = 2;
        newRoot->header.type = BLOCK_TYPE_NODE;

        // split
        root->split(nextNode);

        newRoot->kv[0].value = root->header.index;
        newRoot->kv[0].key = root->kv[0].key;
        newRoot->kv[1].value = nextNode->header.index;
        newRoot->kv[1].key = nextNode->kv[0].key;

        root = newRoot;
        storage.setIndexOfRoot(newRoot->header.index);
    }
    return 1;
}

/*
 * 分裂不是在向全满的节点插入时发生, 而是在节点插入后检测到已满就开始分裂
 * 节点是否已满已经分裂操作, 在父节点插入的递归回溯过程(子节点已经插入完成)中完成, 便于父节点获取新孩子的信息
 */
bool BTree::insert(KeyValue kv, NodeBlock *cur) {
    uint16_t i = cur->find(kv.key);

    if (cur->isLeaf()) {
        // no repetition key so far
        if (kv.key != cur->kv[i].key)
            cur->insert(kv, i);
        else
            cur->kv[i] = kv; //重复key, 替换原来的value
    }
    else {
        if (cur->kv[i].key > kv.key || i == cur->header.count) {
            i--;
        }
        NodeBlock *son = (NodeBlock *)storage.getBlock(cur->kv[i].value);
        if (son == NULL) return false;

        if (!insert(kv, son)) return false;//子节点上插入失败

        // 更新首支针
        // 内部节点的第一个Key也被使用,意义与后面的Key相同
        if (i == 0) { cur->kv[i].key = son->kv[0].key; }

        // 孩子节点插入新节点后变满，分裂孩子节点
        if (son->full()) {
            //            uint32_t index = 0;
            NodeBlock *nextNode = (NodeBlock *)storage.getFreeBlock();
            if (nextNode == NULL) {
                // TODO: failed to allocate a new block
                return false;
            }
            // 分裂
            son->split(nextNode);
            // 在父节点中插入新节点
            KeyValue r;
            r.key = nextNode->kv[0].key;
            r.value = nextNode->header.index;
            cur->insert(r, cur->find(nextNode->kv[0].key));
        }
    }
    return true;
}
void BTree::remove(uint64_t key, NodeBlock *cur) {
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
        NodeBlock *s = (NodeBlock *)storage.getBlock(cur->kv[i].value);
        remove(key, s);

        // update the key in parent
        cur->kv[i].key = s->kv[0].key;

        if (s->geMin()) return;

        // maintain
        if (i > 0) {
            NodeBlock *left = (NodeBlock *)storage.getBlock(cur->kv[i - 1].value);
            if (left->eqMin()) {
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
            NodeBlock *right = (NodeBlock *)storage.getBlock(cur->kv[i + 1].value);
            if (right->eqMin()) {
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

bool BTree::removeByMark(uint64_t key, NodeBlock *cur) {
    if (cur->empty()) return false;
    while (cur && !cur->isLeaf()) {
        uint16_t i = cur->find(key);
        if (cur->kv[i].key > key || i == cur->header.count) i--;
        cur = (NodeBlock *)storage.getBlock(cur->kv[i].value);
    }
    if (cur && cur->isLeaf()) {
        uint16_t i = cur->find(key);
        if (cur->kv[i].key == key) {
            cur->kv[i].value = 0;
            return true;
        }
    }
    return false;
}


int BTree::remove(uint64_t key) {
    /*if (root->empty()) {
        return 0;
    }
    remove(key, root);
    if (!root->isLeaf() && root->header.count == 1) {
        NodeBlock* s = (NodeBlock*)storage.getBlock(root->kv[0].value);
        storage.setIndexOfRoot(s->header.index);
        storage.freeBlock(root->header.index);
        root = s;
    }*/

    return removeByMark(key, root);
}


KeyValue BTree::search(uint64_t key) {
    NodeBlock *cur = root;
    KeyValue res;
    res.value = 0;
    if (root->empty()) return res;
    while (!cur->isLeaf()) {
        uint16_t i = cur->find(key);
        if (cur->kv[i].key > key || i == cur->header.count) i--;
        cur = (NodeBlock *)storage.readBlock(cur->kv[i].value);
    }
    if (cur->isLeaf()) {
        uint16_t i = cur->find(key);
        if (cur->kv[i].key == key) return cur->kv[i];

    }
    return res;
}

std::vector<KeyValue> BTree::search(uint64_t lo, uint64_t hi) {
    NodeBlock *cur = root;
    std::vector<KeyValue> res;
    if (root->empty()) return res;
    while (!cur->isLeaf()) {
        uint16_t i = cur->find(lo);
        if (cur->kv[i].key > lo || i == cur->header.count) i--;
        cur = (NodeBlock *)storage.readBlock(cur->kv[i].value);
    }
    while (cur->isLeaf()) {
        uint16_t i = cur->find(lo);
        for (; i < cur->header.count; i++) {
            if (cur->kv[i].key >= lo && cur->kv[i].key <= hi && cur->kv[i].value != 0)
                res.push_back(cur->kv[i]);
        }
        if (cur->header.next == 0 || cur->kv[i].key > hi) break;
        cur = (NodeBlock *)storage.readBlock(cur->header.next);
    }
    return res;
}

std::vector<KeyValue> BTree::lower(uint64_t lo) {
    NodeBlock *cur = root;
    std::vector<KeyValue> res;
    if (root->empty()) return res;
    while (!cur->isLeaf()) {
        uint16_t i = cur->find(lo);
        if (cur->kv[i].key > lo || i == cur->header.count) i--;
        cur = (NodeBlock *)storage.readBlock(cur->kv[i].value);
    }
    while (cur->isLeaf()) {
        uint16_t i = cur->find(lo);
        for (; i < cur->header.count; i++) {
            if (cur->kv[i].key >= lo && cur->kv[i].value != 0)
                res.push_back(cur->kv[i]);
        }
        if (cur->header.next == 0) break;
        cur = (NodeBlock *)storage.readBlock(cur->header.next);
    }
    return res;
}


std::vector<KeyValue> BTree::upper(uint64_t hi) {
    NodeBlock *cur = root;
    std::vector<KeyValue> res;
    if (root->empty()) return res;
    while (!cur->isLeaf()) {
        cur = (NodeBlock *)storage.readBlock(cur->kv[0].value);
    }
    while (cur->isLeaf()) {
        uint16_t i = 0;
        for (; i < cur->header.count; i++) {
            if (cur->kv[i].key <= hi && cur->kv[i].value != 0)
                res.push_back(cur->kv[i]);
        }
        if (cur->header.next == 0 || cur->kv[i].key > hi) break;
        cur = (NodeBlock *)storage.readBlock(cur->header.next);
    }
    return res;
}

std::vector<KeyValue> BTree::values() {
    NodeBlock *cur = root;
    std::vector<KeyValue> res;
    if (root->empty()) return res;
    while (!cur->isLeaf()) {
        cur = (NodeBlock *)storage.readBlock(cur->kv[0].value);
    }
    while (cur->isLeaf()) {
        uint16_t i = 0;
        for (; i < cur->header.count; i++) {
            if (cur->kv[i].value != 0)
                res.push_back(cur->kv[i]);
        }
        if (cur->header.next == 0) break;
        cur = (NodeBlock *)storage.readBlock(cur->header.next);
    }
    return res;
}
