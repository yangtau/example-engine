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
#define BTREE_DEBUG
uint16_t NodeBlock::size() const {
#ifdef BTREE_DEBUG
    return 4;

#endif  // BTREE_DEBUG
    return (BLOCK_SIZE - ((uint8_t*)this - (uint8_t*)kv)) / sizeofkv();
}

int NodeBlock::find(const void* key) const {
    int lo = 0;
    int hi = count;
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        void* p = (uint8_t*)kv + mid * sizeofkv();
        int res = cmp(p, key);
        if (res == 0)
            return mid;
        else if (res > 0)
            hi = mid;  // key in kv[mid] is bigger than `key`
        else
            lo = mid + 1;
    }
    return -1;
}

int NodeBlock::lub(const void* key) const {
    int lo = 0;
    int hi = count;
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        void* p = (uint8_t*)kv + mid * sizeofkv();
        int res = cmp(p, key);
        if (res < 0)
            lo = mid + 1;  // key in kv[mid] is less than `key`
        else
            hi = mid;
    }
    return lo;
}

const void* NodeBlock::maxKey() const {
    return (uint8_t*)kv + (count - 1) * sizeofkv();
}

const void* NodeBlock::getValue(int index) const {
    return (uint8_t*)kv + index * sizeofkv() + keylen + 1;
}

const void* NodeBlock::getKey(int index) const {
    return (uint8_t*)kv + index * sizeofkv();
}

void NodeBlock::insert(const void* key, const void* value, uint16_t index) {
    assert(index <= count);
    uint8_t* p = kv + (index)*sizeofkv();
    memmove(p + sizeofkv(), p, sizeofkv() * (count - index));

    // copy key
    memcpy(p, key, keylen);
    p += keylen;

    // flag
    *p = 1;
    p++;

    // value
    memcpy(p, value, vallen);

    count++;
}

void NodeBlock::set(uint16_t index,
                    const void* key,
                    const void* value,
                    uint8_t flag) {
    uint8_t* p = kv + index * sizeofkv();

    // copy key
    if (key != NULL)
        memcpy(p, key, keylen);
    p += keylen;

    // flag
    *p = flag;
    p++;

    // value
    if (value != NULL)
        memcpy(p, value, vallen);
}

void NodeBlock::split(NodeBlock* nextNode) {
    // move kv
    // [count/2+1, count-1]
    memcpy(nextNode->kv, kv + ((count + 1) / 2) * sizeofkv(),
           count / 2 * sizeofkv());

    nextNode->count = count / 2;

    count = (count + 1) / 2;

    // chain brother
    // NodeBlock *oldNext = (NodeBlock*)btree.storage.getBlock(header.next);
    /*nextNode->header.next = header.next;
    nextNode->header.last = header.index;

    header.next = nextNode->header.index;
    oldNext->header.last = nextNode->header.index;*/
}

void NodeBlock::merge(NodeBlock* nextNode) {
    memcpy(kv + count * sizeofkv(), nextNode->kv, nextNode->count * sizeofkv());

    count += nextNode->count;

    header.next = nextNode->header.next;
    /*
        NodeBlock *newNext = (NodeBlock*)btree.storage.getBlock(header.next);
        newNext->header.last = header.index;*/
}

void NodeBlock::remove(uint16_t index) {
    assert(index < count);
    memmove(kv + index * sizeofkv(), kv + (index + 1) * sizeofkv(),
            sizeofkv() * (count - index - 1));
    count--;
}

bool NodeBlock::removed(uint16_t index) const {
    uint8_t* p = (uint8_t*)kv + index * sizeofkv() + keylen;
    return *p == 0;
}

void NodeBlock::removeByFlag(uint16_t index) {
    set(index, NULL, NULL, 0);
}

int BTree::Iterator::open(const void* lo, const void* hi) {
    if (opened)
        return 0;  // the iterator has been opened

    if (lo != NULL && hi != NULL && btree.cmp(lo, hi) >= 0)
        return 0;
    // cpy lo, hi
    if (lo != NULL) {
        this->lo = malloc(sizeof(btree.keylen));
        memcpy(this->lo, lo, btree.keylen);
    }
    if (hi != NULL) {
        this->hi = malloc(sizeof(btree.keylen));
        memcpy(this->hi, hi, btree.keylen);
    }

    // locate node
    if (lo != NULL)
        cur = btree.getLeaf(lo);
    else
        cur = btree.getFirstLeaf();
    if (cur == NULL) {
        close();
        return 0;
    }

    // locate index
    index = 0;
    if (lo != NULL)
        index = cur->lub(lo);
    assert(index != cur->count);

    hasNext = true;

    // check
    if (cur->removed(index)) {
        if (next() == 0) {
            close();
            return 0;
        }
    }
    opened = true;
    return 1;
}

void BTree::Iterator::close() {
    if (lo != NULL)
        free(lo);
    if (hi != NULL)
        free(hi);
    lo = NULL;
    hi = NULL;
    cur = NULL;
    hasNext = false;
    opened = false;
}

int BTree::Iterator::next() {
    assert(!cur->empty());

    if (!hasNext) {
        hasNext = false;
        return 0;
    }

    if (index < cur->count - 1) {
        index++;
    } else if (index == cur->count - 1) {
        // move to the next block
        if (cur->header.next == 0) {
            hasNext = false;
            return 0;
        }
        cur = (NodeBlock*)btree.storage.getBlock(cur->header.next);
        index = 0;
    }

    // check the upper bound
    if (hi != NULL) {
        // uint8_t* p = cur->kv + index * cur->sizeofkv();
        if (btree.cmp(cur->getKey(index), hi) >= 0) {  // p>=hi
            hasNext = false;
            return 0;
        }
    }

    // the key may have been removed
    if (cur->removed(index))
        return next();
    return 1;
}

void BTree::Iterator::set(void* value) {
    assert(opened);
    cur->set(index, NULL, value);
}

void BTree::Iterator::get(void* key, void* value) const {
    assert(opened);
    if (key != NULL)
        memcpy(key, cur->getKey(index), cur->keylen);
    if (value != NULL)
        memcpy(value, cur->getValue(index), cur->vallen);
}

void BTree::Iterator::remove() {
    assert(opened);
    cur->removeByFlag(index);
}

BTree::BTree(uint8_t keylen,
             uint8_t vallen,
             Compare cmp,
             StorageManager& storage)
    : keylen(keylen), vallen(vallen), cmp(cmp), storage(storage) {
    if (storage.getIndexOfRoot() == 0) {
        // uint32_t index = 0;
        root = (NodeBlock*)getFreeBlock();
        if (root == NULL) {
            // TODO: error
        }
        storage.setIndexOfRoot(root->header.index);
        root->init(BLOCK_TYPE_LEAF, keylen, vallen);
    } else {
        root = getBlock(storage.getIndexOfRoot());
        if (root == NULL) {
            // TODO: error
        }
    }
}

NodeBlock* BTree::getLeaf(const void* key) {
    int index = root->lub(key);
    if (index == root->count)
        return NULL;
    if (root->leaf())
        return root;

    NodeBlock* node = getBlock(*(uint32_t*)(root->getValue(index)));

    while (!node->leaf()) {
        int index = node->lub(key);
        if (index == node->count)
            return NULL;

        node = getBlock(*(uint32_t*)(node->getValue(index)));
    }
    return node;
}

NodeBlock* BTree::getFirstLeaf() {
    if (root->empty())
        return NULL;
    if (root->leaf())
        return root;
    NodeBlock* node = getBlock(*(uint32_t*)root->getValue(0));
    while (!node->leaf()) {
        node = getBlock(*(uint32_t*)node->getValue(0));
    }
    return node;
}

NodeBlock* BTree::getBlock(uint32_t index) {
    NodeBlock* node = (NodeBlock*)storage.getBlock(index);
    assert(node != NULL);
    node->setCmp(cmp);
    return node;
}

NodeBlock* BTree::getFreeBlock() {
    NodeBlock* node = (NodeBlock*)storage.getFreeBlock();
    assert(node != NULL);
    node->setCmp(cmp);
    return node;
}

int BTree::insert(const void* key, const void* value, NodeBlock* cur) {
    int index = cur->lub(key);
    if (cur->leaf()) {
        // repetition key
        if (index < cur->count && cmp(key, cur->getKey(index)) == 0
            //&& !cur->removed(index)
        )
            cur->set(index, NULL, value);
        else
            cur->insert(key, value, index);
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
    uint32_t blockid = *(uint32_t*)cur->getValue(index);
    NodeBlock* son = getBlock(blockid);

    // insert into son
    if (insert(key, value, son) == 0) {
        return 0;
    }

    // update the key
    if (maxKeyFlag)
        cur->set(index, son->maxKey(), NULL);

    if (son->full()) {
        NodeBlock* next = (NodeBlock*)getFreeBlock();
        next->init(son->header.type, son->keylen, son->vallen);
        son->split(next);

        if (son->header.next != 0) {
            NodeBlock* oldnext = getBlock(son->header.next);
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

    return 1;
}

int BTree::put(const void* key, const void* value) {
    if (root->empty()) {
        root->insert(key, value, 0);
        return 1;
    }
    if (insert(key, value, root) == 0)
        return 0;

    // split root
    if (root->full()) {
        NodeBlock* newRoot = (NodeBlock*)getFreeBlock();  // new root
        NodeBlock* nextNode =
            (NodeBlock*)getFreeBlock();  // next node of old root

        // init of new node
        newRoot->init(BLOCK_TYPE_NODE, keylen, sizeof(uint32_t));
        nextNode->init(root->header.type, keylen, root->vallen);

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

        storage.setIndexOfRoot(newRoot->header.index);
        root = newRoot;
    }
    return 1;
}

int BTree::get(const void* key, void* value) {
    NodeBlock* leaf = getLeaf(key);
    if (leaf == NULL)
        return 0;

    int index = leaf->find(key);
    if (index == -1 || leaf->removed(index)) {
        return 0;
    }

    if (value != NULL)
        memcpy(value, leaf->getValue(index), vallen);
    return 1;
}

// remove by flag
int BTree::remove(const void* key) {
    NodeBlock* leaf = getLeaf(key);
    if (leaf == NULL)
        return 0;

    int index = leaf->find(key);
    if (index == -1 || leaf->removed(index))
        return 0;
    // remove by flag
    leaf->removeByFlag(index);
    return 1;
}

BTree::Iterator* BTree::iterator() {
    return new BTree::Iterator(*this);
}
