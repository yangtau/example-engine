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

//#define BTREE_DEBUG

u16 NodeBlock::size() const {
#ifdef BTREE_DEBUG
    return 32;
#endif // BTREE_DEBUG
    return (BLOCK_SIZE - ((u8 *)this - (u8 *)kv)) / sizeOfItem();
}

int NodeBlock::find(const void *key) const {
    int lo = 0;
    int hi = count;
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        void *p = (u8 *)kv + mid * sizeOfItem();
        int res = btree->cmp(p, key, btree->extraCmpInfo);
        if (res == 0)
            return mid;
        else if (res > 0)
            hi = mid; // key in kv[mid] is bigger than `key`
        else
            lo = mid + 1;
    }
    return -1;
}

int NodeBlock::lub(const void *key) const {
    int lo = 0;
    int hi = count;
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        void *p = (u8 *)kv + mid * sizeOfItem();
        int res = btree->cmp(p, key, btree->extraCmpInfo);
        if (res < 0)
            lo = mid + 1; // key in kv[mid] is less than `key`
        else
            hi = mid;
    }
    return lo;
}

const void *NodeBlock::maxKey() const {
    return (u8 *)kv + (count - 1) * sizeOfItem();
}

const void *NodeBlock::getValue(int index) const {
    return (u8 *)kv + index * sizeOfItem() + keySize + 1;
}

const void *NodeBlock::getKey(int index) const {
    return (u8 *)kv + index * sizeOfItem();
}

void NodeBlock::insert(const void *key, const void *value, u16 index) {
    assert(index <= count);
    u8 *p = kv + (index)*sizeOfItem();
    memmove(p + sizeOfItem(), p, sizeOfItem() * (count - index));

    // copy key
    memcpy(p, key, keySize);
    p += keySize;

    // flag
    *p = 1;
    p++;

    // value
    memcpy(p, value, valSize);

    count++;
}

void NodeBlock::set(u16 index, const void *key, const void *value, u8 flag) {
    u8 *p = kv + index * sizeOfItem();

    // copy key
    if (key != NULL)
        memcpy(p, key, keySize);
    p += keySize;

    // flag
    *p = flag;
    p++;

    // value
    if (value != NULL)
        memcpy(p, value, valSize);
}

void NodeBlock::split(NodeBlock *nextNode) {
    // move kv
    // [count/2+1, count-1]
    memcpy(nextNode->kv, kv + ((count + 1) / 2) * sizeOfItem(),
        count / 2 * sizeOfItem());

    nextNode->count = count / 2;

    count = (count + 1) / 2;
}

void NodeBlock::merge(NodeBlock *nextNode) {
    memcpy(kv + count * sizeOfItem(), nextNode->kv,
        nextNode->count * sizeOfItem());

    count += nextNode->count;

    header.next = nextNode->header.next;
}

void NodeBlock::remove(u16 index) {
    assert(index < count);
    memmove(kv + index * sizeOfItem(), kv + (index + 1) * sizeOfItem(),
        sizeOfItem() * (count - index - 1));
    count--;
}

bool NodeBlock::removed(u16 index) const {
    u8 *p = (u8 *)kv + index * sizeOfItem() + keySize;
    return *p == 0;
}

void NodeBlock::removeByFlag(u16 index) { set(index, NULL, NULL, 0); }

// compare the key with `index`, and `key`
int NodeBlock::compare(u16 index, const void *key) const {
    return btree->cmp(key, this->getKey(index), btree->extraCmpInfo);
}

int BTree::Iterator::locate(const void *key, IterFlag flag) {
    assert(key != NULL);

    cur = btree->getLeaf(key);
    index = cur->lub(key);
    if (index == cur->count) {
        // the `key` is after the last key or the tree is empty
        if (flag == 2 || flag == 4) {
            index--;
            if (cur->removed(index)) {
                return prev();
            }
            else {
                return 1;
            }
        }
        else {
            return 0;
        }
    }
    int res = compare(key);
    switch (flag) {
    case 0: // exact the key
        if (res == 0 && !cur->removed(index))
            return 1;
        else
            return 0;
    case 1: // the key or next
        if (cur->removed(index))
            return next();
        else
            return 1;
    case 2: // the key or prev
        if (res != 0 || cur->removed(index))
            return prev();
        else
            return 1;
    case 3: // after the key
        if (res == 0 || cur->removed(index))
            return next();
        else
            return 1;
    case 4:
        return prev();
    default:
        return 0;
    }
}

int BTree::Iterator::first() {
    cur = btree->getFirstLeaf();
    if (cur->empty()) {
        return 0;
    }
    index = 0;
    if (cur->removed(index))
        return next();
    return 1;
}

int BTree::Iterator::last() {
    cur = btree->getLastLeaf();
    if (cur->empty()) {
        return 0;
    }
    index = cur->count - 1;
    if (cur->removed(index))
        return prev();
    return 1;
}

int BTree::Iterator::next() {
    if (index < cur->count - 1) {
        index++;
    }
    else if (index == cur->count - 1) {
        // move to the next block
        if (cur->header.next == 0) {
            return 0;
        }
        cur = (NodeBlock *)btree->getNodeBlock(cur->header.next);
        index = 0;
    }

    // the key may have been removed
    // if (cur->removed(index)) return next();
    while (cur->removed(index)) {
        if (index < cur->count - 1) {
            index++;
        }
        else if (index == cur->count - 1) {
            // move to the next block
            if (cur->header.next == 0) {
                return 0;
            }
            cur = (NodeBlock *)btree->getNodeBlock(cur->header.next);
            index = 0;
        }
    }
    return 1;
}

int BTree::Iterator::prev() {
    if (index > 0) {
        index--;
    }
    else if (index == 0) {
        // move to the previous block
        if (cur->header.last == 0) {
            return 0;
        }
        cur = (NodeBlock *)btree->getNodeBlock(cur->header.last);
        index = cur->count - 1;
    }
    // the key may have been removed
    if (cur->removed(index))
        return prev();
    return 1;
}
//
// void BTree::Iterator::set(void *value) {
//    cur->set(index, NULL, value);
//
//}

// void BTree::Iterator::get(void *key, void *value) const {
//    // TODO:
//    if (key != NULL) memcpy(key, cur->getKey(index), cur->keySize);
//    if (value != NULL) memcpy(value, cur->getValue(index), cur->valSize);
//}

void BTree::Iterator::remove() { cur->removeByFlag(index); }

int BTree::Iterator::compare(const void *key) const {
    return cur->compare(index, key);
}

const void *BTree::Iterator::getKey() { return cur->getKey(index); }

const void *BTree::Iterator::getValue() {
    if (btree->isConstValueSize) {
        return cur->getValue(index);
    }
    else {
        Location loc = *(Location *)cur->getValue(index);
        return btree->getValue(loc);
    }
}

// BTree
BTree::BTree(u16 keySize, Compare *c, void *p, u16 valueSize)
    : keySize(keySize), cmp(c), extraCmpInfo(p),
    leafValueSize(valueSize >= 256 ? sizeof(LeafValue) : valueSize),
    isConstValueSize(valueSize < 256) {}

int BTree::create(const char *filename) {
    u32 numOfBlock = 32;
    MyFile file;
    if (!file.create(filename, numOfBlock, blockSize))
        return 0;
    if (!file.open(filename))
        return 0;

    // root
    NodeBlock *block = (NodeBlock*)_aligned_malloc(blockSize, blockSize);;
    if (block == NULL) return 0;
    block->init(1, BLOCK_TYPE_LEAF, keySize, leafValueSize);
    if (!file.writeBlock(block, 1, blockSize)) {
        _aligned_free(meta);
        return 0;
    }

    // metadata
    BTreeMetadata *meta = (BTreeMetadata *)block;
    meta->init(0);
    meta->countOfBlock = 2;
    meta->numberOfBlock = numOfBlock;
    meta->root = 1;
    if (!file.writeBlock(meta, 0, blockSize)) {
        _aligned_free(meta);
        return 0;
    }
    _aligned_free(meta);
    return 1;
}

int BTree::open(const char *filename) {
    if (!file.open(filename))
        return 0;
    buffer = new BufferManager(&file, blockSize);
    meta = (BTreeMetadata *)buffer->getBlock(0);
    if (meta == NULL) {
        delete buffer;
        return 0;
    }

    root = getNodeBlock(meta->root);
    if (root == NULL) {
        delete buffer;
        return 0;
    }
    root->setBTree(this);
    return 1;
}

// locate the position the `key` should insert
NodeBlock *BTree::getLeaf(const void *key) {
    int index = root->lub(key);
    if (root->leaf())
        return root;

    if (index == root->count)
        index--;
    NodeBlock *node = getNodeBlock(*(u32 *)(root->getValue(index)));

    while (!node->leaf()) {
        int index = node->lub(key);
        if (index == node->count)
            index--;

        node = getNodeBlock(*(u32 *)(node->getValue(index)));
    }
    return node;
}

NodeBlock *BTree::getFirstLeaf() {
    if (root->leaf())
        return root;
    NodeBlock *node = getNodeBlock(*(u32 *)root->getValue(0));
    while (!node->leaf()) {
        node = getNodeBlock(*(u32 *)node->getValue(0));
    }
    return node;
}

NodeBlock *BTree::getLastLeaf() {
    NodeBlock *node = root;
    while (!node->leaf()) {
        node = getNodeBlock(*(u32 *)node->getValue(node->count - 1));
    }
    return node;
}

NodeBlock *BTree::getNodeBlock(u32 index) {
    assert(index < meta->countOfBlock);
    NodeBlock *block = (NodeBlock *)buffer->getBlock(index);
    if (block != NULL)
        block->setBTree(this);
    return block;
}

NodeBlock *BTree::getFreeNodeBlock(u16 type, u16 keySize, u16 valueSize) {
    if (meta->numberOfBlock == meta->countOfBlock) {
        // resize file
        meta->numberOfBlock *= 2;
        file.resize(meta->numberOfBlock, blockSize);
    }
    NodeBlock *block = (NodeBlock *)buffer->getBlock(meta->countOfBlock);
    // TODO:test
    assert(block->header.reserved == 1);
    if (block != NULL) {
        block->init(meta->countOfBlock, type, keySize, valueSize, this);
        meta->countOfBlock++;
    }
    return block;
}

DataBlock *BTree::getFreeDataBlock() {
    if (meta->numberOfBlock == meta->countOfBlock) {
        // resize file
        meta->numberOfBlock *= 2;
        file.resize(meta->numberOfBlock, blockSize);
    }
    DataBlock *block = (DataBlock *)buffer->getBlock(meta->countOfBlock);
    if (block != NULL) {
        block->init(meta->countOfBlock);
        meta->countOfBlock++;
    }
    return block;
}

// value will be stored in data block
// if `index` is bigger than meta->countOfBlock, insert into a new block.
int BTree::insertValue(u32 index, const void *value, u32 valueSize,
    Location *loc) {
    DataBlock *block = NULL;
    assert(index <= meta->countOfBlock);

    if (index == meta->countOfBlock)
        block = getFreeDataBlock();
    else
        block = (DataBlock *)buffer->getBlock(index);

    if (block == NULL)
        return 0;

    if (block->freeSize() <= valueSize) {
        //        buffer->releaseBlock(index);
        if (block->header.next != 0) {
            // insert into the next block
            return insertValue(block->header.next, value, valueSize, loc);
        }
        else {
            // insert into a new block
            return insertValue(meta->countOfBlock, value, valueSize, loc);
        }
    }

    // insert
    if (!block->insert(value, valueSize, &loc->position))
        return 0;
    loc->index = block->header.index;
    return 1;
}

// copy the value to the buf
const void *BTree::getValue(BTree::Location loc) {
    assert(loc.index < meta->countOfBlock);
    DataBlock *block = (DataBlock *)buffer->getBlock(loc.index, 0);
    if (block == NULL)
        return NULL;
    const void *p = block->get(loc.position);

    // buffer->releaseBlock(loc.index);
    return p;
}

/// @return 0  duplicate key
///         -1 error
int BTree::insert(const void *key, const void *value, u32 valueSize,
    NodeBlock *cur) {
    int index = cur->lub(key);
    if (cur->leaf()) {
        // repetition key
        if (index < cur->count && cur->compare(index, key) == 0) {
            if (cur->removed(index)) {
                if (isConstValueSize) {
                    cur->set(index, NULL, value);
                }
                else {
                    Location loc = *(Location *)cur->getValue(index);
                    if (!insertValue(loc.index, value, valueSize, &loc)) {
                        return -1;
                    }
                    cur->set(index, NULL, &loc);
                }
            }
            else {
                return 0; // duplicate key
            }
        }
        else {
            if (isConstValueSize) {
                cur->insert(key, value, index);
            }
            else {
                Location loc;
                if (!cur->empty())
                    loc = *(Location *)cur->getValue(index - (index == cur->count ? 1 : 0));
                else
                    loc.index = meta->countOfBlock; // insert into new block

                if (!insertValue(loc.index, value, valueSize, &loc)) {
                    return -1;
                }
                cur->insert(key, &loc, index);
            }
        }
        return 1;
    }

    // the max key
    // the key of current node needs to be update only if the flag is true.
    bool maxKeyFlag = false;
    if (index == cur->count) {
        index--;
        maxKeyFlag = true;
    }

    // get son
    NodeBlock *son = getNodeBlock(*(u32 *)cur->getValue(index));

    // insert into son
    int res = insert(key, value, valueSize, son);
    if (res != 1) {
        return res;
    }

    // update the key
    if (maxKeyFlag)
        cur->set(index, son->maxKey(), NULL);

    // split son
    if (son->full()) {
        NodeBlock *next = (NodeBlock *)getFreeNodeBlock(son->header.type,
            son->keySize, son->valSize);
        son->split(next);

        if (son->header.next != 0) {
            NodeBlock *oldnext = getNodeBlock(son->header.next);
            oldnext->header.last = next->header.index;
            next->header.next = son->header.next;
        }

        son->header.next = next->header.index;
        next->header.last = son->header.index;

        // max key of `next` is the same with the previous max key
        // `next` repalce the son in `cur`
        cur->set(index, NULL, &next->header.index);

        // insert the `son` with the new max key
        cur->insert(son->maxKey(), &son->header.index);
    }

    //    buffer->releaseBlock(son->header.index);
    return 1;
}

int BTree::put(const void *key, const void *value, u32 valueSize) {
    int res = insert(key, value, valueSize, root);
    if (res != 1)
        return res;
    meta->countOfItem++;

    // split root
    if (root->full()) {
        NodeBlock *newRoot = (NodeBlock *)getFreeNodeBlock(
            BLOCK_TYPE_NODE, keySize, nodeValueSize); // new root
        NodeBlock *nextNode =
            (NodeBlock *)getFreeNodeBlock(root->header.type, keySize,
                root->valSize); // next node of old root

// the max key of current root is the max key of the next node after
// splitting.
        newRoot->set(1, root->maxKey(), &nextNode->header.index);

        // split
        root->split(nextNode);

        // chain
        root->header.next = nextNode->header.index;
        nextNode->header.last = root->header.index;

        newRoot->set(0, root->maxKey(), &root->header.index);
        newRoot->count = 2;

        meta->root = newRoot->header.index;
        root = newRoot;
    }
    return 1;
}