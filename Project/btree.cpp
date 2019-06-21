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

int BTreeNode::next() {
    if (block->header.next == 0) return 0;

    NodeBlock *next = (NodeBlock*)btree.storage.getBlock(block->header.next);
    block = next;

    return 1;
}

int BTreeNode::last() {
    if (block->header.last == 0) return 0;

    NodeBlock *last = (NodeBlock*)btree.storage.getBlock(block->header.last);
    block = last;

    return 1;
}

int BTreeNode::find(const void* key) const {
    int lo = 0;
    int hi = block->count;
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        void * p = block->kv + mid * sizeofkv();
        int res = btree.cmp(p, key);
        if (res == 0) return mid;
        else if (res > 0) hi = mid; // key in kv[mid] is bigger than `key`
        else lo = mid + 1;
    }
    return -1;
}

int BTreeNode::lub(const void * key) const {
    int lo = 0;
    int hi = block->count;
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        void * p = block->kv + mid * sizeofkv();
        int res = btree.cmp(p, key);
        if (res < 0) lo = mid + 1; // key in kv[mid] is less than `key`
        else hi = mid;
    }
    void * p = block->kv + lo * sizeofkv();
    if (btree.cmp(p, key) >= 0) return lo;
    return block->count;
}

const void * BTreeNode::maxKey() const {
    return block->kv + (block->count - 1) * sizeofkv();
}

const void * BTreeNode::getValue(int index) const {
    return block->kv + index * sizeofkv() + keylen + 1;
}

const void * BTreeNode::getKey(int index) const {
    return block->kv + index * sizeofkv();
}

void BTreeNode::insert(const void* key, const void* value, uint16_t index) {
    assert(index <= block->count);
    uint8_t *p = block->kv + (index)*sizeofkv();
    memmove(p + sizeofkv(), p, sizeofkv() * (block->count - index));

    // copy key
    memcpy(p, key, keylen);
    p += keylen;

    // flag 
    *p = 1;
    p++;

    // value
    memcpy(p, value, vallen);

    block->count++;
}

void BTreeNode::setKey(const void * key, uint16_t index) {
    uint8_t *p = block->kv + (index)*sizeofkv();
    // value
    memcpy(p, key, keylen);
}

void BTreeNode::setValue(const void * value, uint16_t index) {
    uint8_t *p = block->kv + (index)*sizeofkv() + keylen + 1;
    // value
    memcpy(p, value, vallen);
}

void BTreeNode::split(NodeBlock *nextNode) {
    //nextNode->header.type = block->header.type;

    // move kv
    // [count/2+1, count-1]
    memcpy(nextNode->kv,
        block->kv + (block->count / 2 + 1) *sizeofkv(),
        block->count / 2 * sizeofkv());

    block->count = (block->count + 1) / 2;

    nextNode->count = block->count / 2;

    // chain brother
    NodeBlock *oldNext = (NodeBlock*)btree.storage.getBlock(block->header.next);

    nextNode->header.next = block->header.next;
    nextNode->header.last = block->header.index;

    block->header.next = nextNode->header.index;
    oldNext->header.last = nextNode->header.index;
}

void BTreeNode::merge(NodeBlock *nextNode) {
    memcpy(block->kv + block->count*sizeofkv(),
        nextNode->kv,
        nextNode->count * sizeofkv());

    block->count += nextNode->count;

    block->header.next = nextNode->header.next;

    NodeBlock *newNext = (NodeBlock*)btree.storage.getBlock(block->header.next);
    newNext->header.last = block->header.index;
}


void BTreeNode::remove(uint16_t index) {
    assert(index < block->count);
    memmove(block->kv + index * sizeofkv(),
        block->kv + (index + 1)*sizeofkv(),
        sizeofkv() * (block->count - index - 1));
    block->count--;
}



int BTreeIterator::open(const void * lo, const void * hi) {
    // cpy lo, hi
    this->lo = malloc(sizeof(btree.keylen));
    this->hi = malloc(sizeof(btree.keylen));
    memcpy(this->lo, lo, btree.keylen);
    memcpy(this->hi, hi, btree.keylen);

    // locate
    cur = btree.getLeaf(lo);
    index = cur->find(lo);
    if (index = -1) {
        delete cur;
        return 0;
    }
    return 1;
}

void BTreeIterator::close() {
    free(lo);
    free(hi);
    lo = NULL;
    hi = NULL;
    delete cur;
    cur = NULL;
}

int BTreeIterator::next() {
    if (cur->empty()) return 0;

    // current position is not in the end of the block
    if (index < cur->block->count - 1) {
        index++;
        uint8_t *p = cur->block->kv + index * cur->sizeofkv();
        if (btree.cmp(p, hi) >= 0) { // p>=hi
            index--;
            return 0;
        }
        return 1;
    }

    // next block
    if (index == cur->block->count) {
        if (cur->next() == 0) {
            return 0;
        }
        int t = index;
        index = 0;
        uint8_t *p = cur->block->kv + index * cur->sizeofkv();
        if (btree.cmp(p, hi) >= 0) { // p>=hi
            cur->last();
            index = t;
            return 0;
        }
        return 1;
    }
}

int BTreeIterator::forward() {
    if (cur->empty()) return 0;

    // the first in the block
    if (index == 0) {
        if (cur->last() == 0) {
            return 0;
        }
        int t = index;
        index = cur->block->count - 1;
        uint8_t *p = cur->block->kv + index * cur->sizeofkv();
        if (btree.cmp(p, hi) < 0) { // p>=hi
            cur->next();
            index = t;
            return 0;
        }
        return 1;
    }

    index--;
    uint8_t *p = cur->block->kv + index * cur->sizeofkv();
    if (btree.cmp(p, hi) < 0) { // p>=hi
        index++;
        return 0;
    }
    return 1;
}

void BTreeIterator::set(void * value) {
    memcpy(cur->block->kv + index * cur->sizeofkv() + 1 + cur->keylen,
        value, cur->vallen);
}

void BTreeIterator::get(void * key, void * value) const {
    memcpy(key, cur->block->kv + index * cur->sizeofkv(), cur->keylen);
    memcpy(value, cur->block->kv + index * cur->sizeofkv() + 1 + cur->keylen, cur->vallen);
}

void BTreeIterator::remove() {
    uint8_t *p = cur->block->kv + index * cur->sizeofkv() + cur->keylen;
    *p = 0;
}


// TODO: return null 
BTreeNode * BTree::getLeaf(const void * key) {
    if (root->leaf()) return root;
    int index = root->lub(key);
    if (index == root->count()) return NULL;
    uint32_t blockid = *(uint32_t*)(root->getValue(index));
    BTreeNode* node = getNode(blockid);

    while (!node->leaf()) {
        int index = node->lub(key);
        assert(index != root->count());

        uint32_t blockid = *(uint32_t*)(node->getValue(index));
        delete node;
        node = getNode(blockid);
    }
    return node;
}

BTreeNode * BTree::getNode(uint32_t index) {
    return new BTreeNode(*this, storage.getBlock(index));
}


BTree::BTree(uint8_t keylen, uint8_t vallen, Compare cmp, StorageManager & storage) :
    keylen(keylen), vallen(vallen), cmp(cmp), storage(storage) {
    NodeBlock* rootBlock = NULL;
    if (storage.getIndexOfRoot() == 0) {
        // uint32_t index = 0;
        rootBlock = (NodeBlock *)storage.getFreeBlock();
        if (root == NULL) {
            // TODO: error
        }
        storage.setIndexOfRoot(rootBlock->header.index);
        rootBlock->init(BLOCK_TYPE_LEAF);
    }
    else {
        rootBlock = (NodeBlock *)storage.getBlock(storage.getIndexOfRoot());
        if (rootBlock == NULL) {
            // TODO: error
        }
    }
    if (rootBlock != NULL) root = new BTreeNode(*this, rootBlock);
}

int BTree::insert(const void * key, const void * value, BTreeNode * cur) {
    int index = cur->lub(key);
    if (cur->leaf()) {
        // repetition key
        if (index < cur->count() && cmp(key, cur->getKey(index)) == 0)
            cur->setValue(value, index);
        else
            cur->insert(key, value, index);
        return 1;
    }

    // the biggest key
    if (index == cur->count())
        index--;
    uint32_t blockid = *(uint32_t*)cur->getValue(index);
    BTreeNode *son = getNode(blockid);
    if (insert(key, value, son) == 0) {
        delete son;
        return 0;
    }

    cur->setKey(son->maxKey(), index);

    if (son->full()) {
        NodeBlock *next = (NodeBlock *)storage.getFreeBlock();
        next->init(son->block->header.type);
        son->split(next);
        BTreeNode nxt = BTreeNode(*this, next);

        // max key of `next` is in `cur`
        cur->insert(nxt.maxKey(), &(next->header.index), cur->lub(nxt.maxKey()));
    }

    delete son;
    return 1;
}


int BTree::put(const void * key, const void * value) {
    if (root->empty()) {
        root->insert(key, value, 0);
        return 1;
    }
    if (insert(key, value, root) == 0)
        return 0;

    // 
    if (root->full()) {
        NodeBlock *newRoot = (NodeBlock *)storage.getFreeBlock();// new root
        NodeBlock *nextNode = (NodeBlock *)storage.getFreeBlock();// next node of old root
        BTreeNode *node = new BTreeNode(*this, newRoot);

        // init of new node
        newRoot->init(BLOCK_TYPE_NODE);
        nextNode->init(root->block->header.type);

        // the max key of current root is the max key of the next node after split
        node->insert(root->maxKey(), &nextNode->header.index, 1);
        
        // split
        root->split(nextNode);
        
        node->insert(root->maxKey(), &root->block->header.index, 0);


        storage.setIndexOfRoot(newRoot->header.index);
    }
}

int BTree::get(const void * key, void * value) {
    BTreeNode *leaf = getLeaf(key);
    int index = leaf->find(key);
    if (index == -1) {
        delete leaf;
        return 0;
    }
    memcpy(value, leaf->getValue(index), vallen);
    return 1;
}

// remove by flag
int BTree::remove(const void * key) {
    BTreeNode *leaf = getLeaf(key);
    int index = leaf->find(key);
    if (index == -1) return 0;
    // remove by flag
    uint8_t *p = leaf->block->kv + index * leaf->sizeofkv() + keylen;
    *p = 0;
    return 1;
}

BTreeIterator BTree::iterator() {
    return  BTreeIterator(*this);
}




//
//BTree::BTree(StorageManager &s) : root(NULL), storage(s) {
//    if (storage.getIndexOfRoot() == 0) {
//        // uint32_t index = 0;
//        root = (NodeBlock *)storage.getFreeBlock();
//        if (root == NULL) {
//            // TODO: 
//        }
//        storage.setIndexOfRoot(root->header.index);
//        root->header.count = 0;
//        root->header.type = BLOCK_TYPE_LEAF;
//    }
//    else {
//        root = (NodeBlock *)storage.getBlock(storage.getIndexOfRoot());
//        if (root == NULL) {
//            // TODO: 
//        }
//    }
//}
//
///*
// * b+tree 高度变化在这个函数中发生
// * bool insert(KeyValue, NodeBlock*) 只会发生子节点的分裂, 不会使得bTree变高
// * 如果插入完成后, root节点变满, 分裂root, 并且生成新的root
// */
//int BTree::insert(KeyValue kv) {
//    if (root->empty()) {
//        root->insert(kv, 0);
//        return 1;
//    }
//    if (!insert(kv, root)) return false;
//
//    // root is full
//    if (root->full()) {
//        NodeBlock *newRoot = (NodeBlock *)storage.getFreeBlock();// new root
//        NodeBlock *nextNode = (NodeBlock *)storage.getFreeBlock();// next node of old root
//        if (newRoot == NULL || nextNode == NULL) {
//            // TODO: error
//            return false;
//        }
//        // init of new root
//        newRoot->header.count = 2;
//        newRoot->header.type = BLOCK_TYPE_NODE;
//
//        // split
//        root->split(nextNode);
//
//        newRoot->kv[0].value = root->header.index;
//        newRoot->kv[0].key = root->kv[0].key;
//        newRoot->kv[1].value = nextNode->header.index;
//        newRoot->kv[1].key = nextNode->kv[0].key;
//
//        root = newRoot;
//        storage.setIndexOfRoot(newRoot->header.index);
//    }
//    return 1;
//}
//
///*
// * 分裂不是在向全满的节点插入时发生, 而是在节点插入后检测到已满就开始分裂
// * 节点是否已满已经分裂操作, 在父节点插入的递归回溯过程(子节点已经插入完成)中完成, 便于父节点获取新孩子的信息
// */
//bool BTree::insert(KeyValue kv, NodeBlock *cur) {
//    uint16_t i = cur->find(kv.key);
//
//    if (cur->isLeaf()) {
//        // no repetition key so far
//        if (kv.key != cur->kv[i].key)
//            cur->insert(kv, i);
//        else
//            cur->kv[i] = kv; //重复key, 替换原来的value
//    }
//    else {
//        if (cur->kv[i].key > kv.key || i == cur->header.count) {
//            i--;
//        }
//        NodeBlock *son = (NodeBlock *)storage.getBlock(cur->kv[i].value);
//        if (son == NULL) return false;
//
//        if (!insert(kv, son)) return false;//子节点上插入失败
//
//        // 更新首支针
//        // 内部节点的第一个Key也被使用,意义与后面的Key相同
//        if (i == 0) { cur->kv[i].key = son->kv[0].key; }
//
//        // 孩子节点插入新节点后变满，分裂孩子节点
//        if (son->full()) {
//            //            uint32_t index = 0;
//            NodeBlock *nextNode = (NodeBlock *)storage.getFreeBlock();
//            if (nextNode == NULL) {
//                // TODO: failed to allocate a new block
//                return false;
//            }
//            // 分裂
//            son->split(nextNode);
//            // 在父节点中插入新节点
//            KeyValue r;
//            r.key = nextNode->kv[0].key;
//            r.value = nextNode->header.index;
//            cur->insert(r, cur->find(nextNode->kv[0].key));
//        }
//    }
//    return true;
//}
//
//void BTree::remove(uint64_t key, NodeBlock *cur) {
//    uint16_t i = cur->find(key);
//
//    if (cur->isLeaf()) {
//        if (cur->kv[i].key > key) return;
//        else {
//            cur->remove(i);
//        }
//    }
//    else {
//        if (cur->kv[i].key > key || i == cur->header.count) {
//            i--;
//        }
//        NodeBlock *s = (NodeBlock *)storage.getBlock(cur->kv[i].value);
//        remove(key, s);
//
//        // update the key in parent
//        cur->kv[i].key = s->kv[0].key;
//
//        if (s->geMin()) return;
//
//        // maintain
//        if (i > 0) {
//            NodeBlock *left = (NodeBlock *)storage.getBlock(cur->kv[i - 1].value);
//            if (left->eqMin()) {
//                // merge
//                left->merge(s);
//                cur->remove(i);
//
//                storage.freeBlock(s->header.index);
//            }
//            else {
//                s->insert(left->kv[left->header.count - 1], 0);
//                left->remove(left->header.count - 1);
//            }
//        }
//        else {
//            NodeBlock *right = (NodeBlock *)storage.getBlock(cur->kv[i + 1].value);
//            if (right->eqMin()) {
//                // merge
//                s->merge(right);
//                cur->remove(i + 1);
//
//                storage.freeBlock(right->header.index);
//            }
//            else {
//                s->insert(right->kv[0], s->header.count);
//                right->remove(0);
//                cur->kv[i + 1].key = right->kv[0].key;
//            }
//        }
//    }
//}
//
//bool BTree::removeByMark(uint64_t key, NodeBlock *cur) {
//    if (cur->empty()) return false;
//    while (cur && !cur->isLeaf()) {
//        uint16_t i = cur->find(key);
//        if (cur->kv[i].key > key || i == cur->header.count) i--;
//        cur = (NodeBlock *)storage.getBlock(cur->kv[i].value);
//    }
//    if (cur && cur->isLeaf()) {
//        uint16_t i = cur->find(key);
//        if (cur->kv[i].key == key) {
//            cur->kv[i].value = 0;
//            return true;
//        }
//    }
//    return false;
//}
//
//
//int BTree::remove(uint64_t key) {
//    /*if (root->empty()) {
//        return 0;
//    }
//    remove(key, root);
//    if (!root->isLeaf() && root->header.count == 1) {
//        NodeBlock* s = (NodeBlock*)storage.getBlock(root->kv[0].value);
//        storage.setIndexOfRoot(s->header.index);
//        storage.freeBlock(root->header.index);
//        root = s;
//    }*/
//
//    return removeByMark(key, root);
//}
//
//
//KeyValue BTree::search(uint64_t key) {
//    NodeBlock *cur = root;
//    KeyValue res;
//    res.value = 0;
//    if (root->empty()) return res;
//    while (!cur->isLeaf()) {
//        uint16_t i = cur->find(key);
//        if (cur->kv[i].key > key || i == cur->header.count) i--;
//        cur = (NodeBlock *)storage.readBlock(cur->kv[i].value);
//    }
//    if (cur->isLeaf()) {
//        uint16_t i = cur->find(key);
//        if (cur->kv[i].key == key) return cur->kv[i];
//
//    }
//    return res;
//}
//
//std::vector<KeyValue> BTree::search(uint64_t lo, uint64_t hi) {
//    NodeBlock *cur = root;
//    std::vector<KeyValue> res;
//    if (root->empty()) return res;
//    while (!cur->isLeaf()) {
//        uint16_t i = cur->find(lo);
//        if (cur->kv[i].key > lo || i == cur->header.count) i--;
//        cur = (NodeBlock *)storage.readBlock(cur->kv[i].value);
//    }
//    while (cur->isLeaf()) {
//        uint16_t i = cur->find(lo);
//        for (; i < cur->header.count; i++) {
//            if (cur->kv[i].key >= lo && cur->kv[i].key <= hi && cur->kv[i].value != 0)
//                res.push_back(cur->kv[i]);
//        }
//        if (cur->header.next == 0 || cur->kv[i].key > hi) break;
//        cur = (NodeBlock *)storage.readBlock(cur->header.next);
//    }
//    return res;
//}
//
//std::vector<KeyValue> BTree::lower(uint64_t lo) {
//    NodeBlock *cur = root;
//    std::vector<KeyValue> res;
//    if (root->empty()) return res;
//    while (!cur->isLeaf()) {
//        uint16_t i = cur->find(lo);
//        if (cur->kv[i].key > lo || i == cur->header.count) i--;
//        cur = (NodeBlock *)storage.readBlock(cur->kv[i].value);
//    }
//    while (cur->isLeaf()) {
//        uint16_t i = cur->find(lo);
//        for (; i < cur->header.count; i++) {
//            if (cur->kv[i].key >= lo && cur->kv[i].value != 0)
//                res.push_back(cur->kv[i]);
//        }
//        if (cur->header.next == 0) break;
//        cur = (NodeBlock *)storage.readBlock(cur->header.next);
//    }
//    return res;
//}
//
//
//std::vector<KeyValue> BTree::upper(uint64_t hi) {
//    NodeBlock *cur = root;
//    std::vector<KeyValue> res;
//    if (root->empty()) return res;
//    while (!cur->isLeaf()) {
//        cur = (NodeBlock *)storage.readBlock(cur->kv[0].value);
//    }
//    while (cur->isLeaf()) {
//        uint16_t i = 0;
//        for (; i < cur->header.count; i++) {
//            if (cur->kv[i].key <= hi && cur->kv[i].value != 0)
//                res.push_back(cur->kv[i]);
//        }
//        if (cur->header.next == 0 || cur->kv[i].key > hi) break;
//        cur = (NodeBlock *)storage.readBlock(cur->header.next);
//    }
//    return res;
//}
//
//std::vector<KeyValue> BTree::values() {
//    NodeBlock *cur = root;
//    std::vector<KeyValue> res;
//    if (root->empty()) return res;
//    while (!cur->isLeaf()) {
//        cur = (NodeBlock *)storage.readBlock(cur->kv[0].value);
//    }
//    while (cur->isLeaf()) {
//        uint16_t i = 0;
//        for (; i < cur->header.count; i++) {
//            if (cur->kv[i].value != 0)
//                res.push_back(cur->kv[i]);
//        }
//        if (cur->header.next == 0) break;
//        cur = (NodeBlock *)storage.readBlock(cur->header.next);
//    }
//    return res;
//}
//

