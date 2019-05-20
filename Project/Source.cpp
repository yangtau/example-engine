#include <iostream>
#include <string>
#include <vector>
#include <inttypes.h>
#include "Windows.h"
#include <assert.h>
#include <stdlib.h>
#include <cstring>
#include <map>

using std::vector;
using std::string;




// Block type
#define BLOCK_TYPE_FREE 0  // free block
#define BLOCK_TYPE_DATA 1  // data block
#define BLOCK_TYPE_META 2  // metadata block
#define BLOCK_TYPE_LEAF 3  // leaf  block
#define BLOCK_TYPE_NODE 4  // interior node block or root block
#define BLOCK_TYPE_LOG 5   // log block

#pragma pack(1)

struct RecordHeader {
    uint16_t size;       // bytes of data
    uint16_t timestamp;  // TODO: size of timestamp
};

struct Record {
    RecordHeader header;
    //uint8_t* getData();
    uint8_t data[1];
};

/// 
// @brief
// header of block
// 96
struct BlockHeader {
    uint32_t magic = 0xc1c6f01e;
    uint16_t type : 3;  // type of block
    uint16_t reserved : 13;
    uint16_t checksum;  
    uint16_t count;     
    uint16_t free;
    uint32_t next;      // index of next block
    uint32_t index;     // index of this block

    uint16_t compute();  // compute checksum and set it
    int check();         // check checksum
};

struct Tailer {
    uint16_t slots[1];  
};

#pragma pack()

const uint32_t BLOCK_SIZE = 4096;

///
// @brief
// block of record
struct RecordBlock {
    BlockHeader header;

    Tailer* getTailer();

    void init();

    int addRecord(Record* record, uint32_t *position);

    int delRecord(uint32_t position);

    Record* getRecord(uint32_t position);

    int updateRecord(uint32_t position, Record* record);

    bool full();
};

///
// @brief
// metadata blcok
struct MetaBlock {
    BlockHeader header;
    uint32_t free; // list of free block
    uint32_t root; // root of b+tree
    uint32_t count; // num of blocks
    uint32_t idle; // num of idle blocks
};




uint16_t BlockHeader::compute() {
    // the max sum 4094/2 * 2^16 < 2^32
    uint32_t sum = 0;
    // Size of block is even, so there is no need to handle odd-sized block
    for (int i = 0; i < BLOCK_SIZE / 2; i++) {
        sum += ((uint16_t*)this)[i];  // get two bytes
    }
    sum -= this->checksum;  // substract checksum from sum
    // Fold to get ones-complement result
    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    return checksum = ~sum;
}

int BlockHeader::check() {
    uint32_t sum = 0;
    for (int i = 0; i < BLOCK_SIZE / 2; i++) {
        sum += ((uint16_t*)this)[i];  // get two bytes
    }
    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    return sum == 0xffff;
}

Tailer* RecordBlock::getTailer() {
    // `BLOCK_SIZE - 2 * header.count` is offset of the tailor
    return (Tailer*)((uint8_t*)(this) + BLOCK_SIZE - 2 * header.count);
}

void RecordBlock::init() {
    header.count = 0;
    header.free = sizeof(BlockHeader);
    header.next = 0;
}

int RecordBlock::delRecord(uint32_t position) {
    if (position > header.count) {
        return false;
    }

    position = header.count - position - 1;

    uint16_t offset = getTailer()->slots[position];
    if (offset == 0) return false;

    Record* record = (Record*)((uint8_t*)this + offset);
    uint16_t size = record->header.size + sizeof(RecordHeader);
    getTailer()->slots[position] = 0;  // pointer to 0
    // move memory, `offset + size - header.free` is the size to move
    memmove(record, (uint8_t*)record + size, header.free - offset + size);
    header.free -= size;
    // modify offset table (tailer)
    for (uint16_t i = 0; i < header.count; i++) {
        if (getTailer()->slots[i] > offset) {
            getTailer()->slots[i] -= size;
        }
    }
    return true;
}

int RecordBlock::addRecord(Record* record, uint32_t *position) {
    uint16_t size = record->header.size + sizeof(RecordHeader);
    // `BLOCK_SIZE - 2 * header.count` is offset of the tailor, 2 bytes for
    // offset
    if (size + 2 + header.free >= BLOCK_SIZE - 2 * header.count) {
        // memory is not enough, add to next block
        return false;
    }
    // copy header record
    memcpy((uint8_t*)this + header.free, record, sizeof(RecordHeader));
    // *(RecordHeader*)((uint8_t*)this + header.free) = record->header;
    // copy data of record
    memcpy((uint8_t*)this + header.free + sizeof(RecordHeader),
        record->data, record->header.size);
    // order of following operations is important !!!
    header.count++;
    getTailer()->slots[0] = header.free;  // getTailer() is calculated by count,
                                          // so count must be update before this
    header.free += size;
    if (position != NULL) *position = header.count - 1;
    return true;
}

Record* RecordBlock::getRecord(uint32_t position) {

    if (position >= header.count) {
        // todo
        return NULL;
    }

    // TODO
    position = header.count - 1 - position;
    uint16_t offset = getTailer()->slots[position];
    // if the record is removed, the offset is 0
    if (offset == 0) return NULL;
    Record* res = (Record*)((uint8_t*)this + offset);
    // res->data = (uint8_t*)this + offset + sizeof(RecordHeader);
    return res;
}

int RecordBlock::updateRecord(uint32_t position, Record* record) {

    // remove the old and insert the new record
    Record* old = getRecord(position);  // get the old record
    if (old == NULL) {
        // the position of the old record is false
        return false;
    }
    // TODO
    position = header.count - 1 - position;

    if (old->header.size == record->header.size) {
        old->header = record->header;
        memcpy((uint8_t*)old + sizeof(RecordHeader), record->data,
            record->header.size);
        return true;
    }

    if (record->header.size - old->header.size + header.free >= BLOCK_SIZE - 2 * header.count) {
        // memory is not enough, add to next block
        return false;
    }

    uint16_t offset = getTailer()->slots[position];
    if (offset == 0) return false;

    uint16_t size = old->header.size + sizeof(RecordHeader);
    // move memory, `offset + size - header.free` is the size to move
    memmove(old, (uint8_t*)old + size, header.free - offset - size);
    header.free -= size;
    // modify offset table (tailer)
    for (uint16_t i = 0; i < header.count; i++) {
        if (getTailer()->slots[i] > offset) {
            getTailer()->slots[i] -= size;
        }
    }
    memcpy((uint8_t*)this + header.free, record, sizeof(RecordHeader));
    // *(RecordHeader*)((uint8_t*)this + header.free) = record->header;
    // copy data of record
    memcpy((uint8_t*)this + header.free + sizeof(RecordHeader),
        record->data, record->header.size);
    getTailer()->slots[position] = header.free;  // point to new position
    header.free += sizeof(RecordHeader) + record->header.size;
    return true;
}

bool RecordBlock::full() {
    return false;
}

class BufferManager {

public:
    void *allocateBlock();
    void freeBlock(void*);
};


void *BufferManager::allocateBlock() {
    void *p = _aligned_malloc(BLOCK_SIZE, BLOCK_SIZE);
    if (p != NULL) VirtualLock(p, BLOCK_SIZE);
    return p;
}

void BufferManager::freeBlock(void *_block) {

    _aligned_free(_block);
}



///
// @brief
// File class
class File {
private:
    HANDLE handle;
public:

    File() : handle(INVALID_HANDLE_VALUE) {}

    ~File() {
        if (handle != INVALID_HANDLE_VALUE)
            CloseHandle(handle);
    }

    int create(const char* path, uint32_t num);

    // `index` is the order the block in the file
    bool readBlock(uint32_t index, void *block);

    // write `block` to file
    bool writeBlock(uint32_t index, void* block);

    bool resize(uint32_t num);
};


bool File::readBlock(uint32_t index, void *block) {
    assert(block != NULL);

    // offset
    uint64_t offset = index * BLOCK_SIZE;  // the position of the block
    if (SetFilePointer(handle, offset, NULL, FILE_BEGIN) != offset)
        return false;

    // read
    unsigned long bytesToRead = -1;
    if (!ReadFile(handle, block, BLOCK_SIZE, &bytesToRead, NULL))
        return false;

    return true;
}

int File::create(const char *path, uint32_t num) {
    // open file
    handle = CreateFileA((LPCSTR)path, GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        FILE_FLAG_NO_BUFFERING, nullptr);
    
    if (handle == INVALID_HANDLE_VALUE) {
        // the file does not exist
        // create file
        handle = CreateFileA((TCHAR *)path, GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_DELETE, nullptr, CREATE_ALWAYS,
            FILE_FLAG_NO_BUFFERING, nullptr);
        if (handle == INVALID_HANDLE_VALUE)
            return 0;

        // set the size of file, `BLOCK_SIZE * num` is the size of file
        BOOL res = SetFilePointer(handle, BLOCK_SIZE * num, NULL, FILE_BEGIN) &&
            SetEndOfFile(handle);
        if (!res)
            return 0;

        return 1;
    }
    else return 2;

}

bool File::writeBlock(uint32_t index, void *_block) {
    RecordBlock *block = (RecordBlock *)_block;
    // offset
    uint64_t offset = index * BLOCK_SIZE;  // the position of the block

    if (SetFilePointer(handle, offset, NULL, FILE_BEGIN) != offset)
        return false;

    // compute checksum
    block->header.compute();

    // write
    unsigned long bytesToWrite = -1;
    return WriteFile(handle, block, BLOCK_SIZE, &bytesToWrite, NULL) &&
        bytesToWrite == BLOCK_SIZE;
   
}

bool File::resize(uint32_t num) {
    return SetFilePointer(handle, BLOCK_SIZE * num, NULL, FILE_BEGIN) &&
        SetEndOfFile(handle);
}



///
// @brief
// storage manager
class StorageManager {
private:
    // `header.reserved` is used as a flag indicating whether the buffer is clean
    std::map<uint32_t, RecordBlock *> buffers;

    File file;

    BufferManager bufferManager;

    MetaBlock *meta;

    bool initFile();
public:
    StorageManager(const char *path);

    ~StorageManager();

    void *getBlock(uint32_t index);

    const void *readBlock(uint32_t index);

    void* getFreeBlock(uint32_t *index);

    void freeBlock(uint32_t index);

    bool save();

    uint32_t getIndexOfRoot();

    void setIndexOfRoot(uint32_t);
};


static const uint32_t NUM_BLOCK = 64;

StorageManager::StorageManager(const char * path) :meta(NULL) {
    int res = file.create(path, NUM_BLOCK);
    if (res == 0) {
        //TODO: failed to create file
    }
    else if (res == 1) {
        initFile();
    }
    meta = (MetaBlock*)getBlock(0);
}

StorageManager::~StorageManager() {
    save();
    for (auto &x : buffers) {
        bufferManager.freeBlock(x.second);
    }
}

void *StorageManager::getBlock(uint32_t index) {
    RecordBlock *block = buffers[index];
    if (block == NULL || block->header.index != index) {
        block = (RecordBlock*)bufferManager.allocateBlock();
        if (!file.readBlock(index, block))
            return NULL;
        buffers[index] = block;
    }
    if (block != NULL) block->header.reserved = 1;
    return block;
}

const void * StorageManager::readBlock(uint32_t  index) {
    RecordBlock *block = buffers[index];
    if (block == NULL || block->header.index != index) {
        block = (RecordBlock*)bufferManager.allocateBlock();
        if (!file.readBlock(index, block))
            return NULL;
        buffers[index] = block;
    }
    return block;
}

// TODO: remove `index`
void * StorageManager::getFreeBlock(uint32_t *index) {
    if (meta->idle == 0 || meta->free == 0) {
        meta->count *= 2;
        file.resize(meta->count);

        // free block list
        for (uint32_t i = meta->count / 2; i < meta->count; i++) {
            RecordBlock *block = (RecordBlock*)getBlock(i);
            if (block == NULL) return false;
            block->header.magic = 0xc1c6f01e;
            block->header.index = i;
            block->header.type = BLOCK_TYPE_FREE;
            // add to free lsit
            block->header.next = meta->free;
            meta->free = i;
        }
        //save();
    }
    RecordBlock *b = (RecordBlock *)readBlock(meta->free);
    if (b == NULL) return NULL;
    if (index != NULL) *index = meta->free;
    b->header.reserved = 1;

    // remove from free list
    meta->free = b->header.next;
    meta->idle--;
    b->header.next = 0;

    return b;
}

void StorageManager::freeBlock(uint32_t index) {
    RecordBlock *b = (RecordBlock *)readBlock(index);

    b->header.next = meta->free;
    meta->free = index;
    meta->idle++;
}

bool StorageManager::save() {
    for (auto &x : buffers) {
        if (x.second->header.reserved == 1) {
            if (!file.writeBlock(x.second->header.index, x.second))
                return false;
            x.second->header.reserved = 0;
        }
    }
    return true;
}

uint32_t StorageManager::getIndexOfRoot() {
    return meta->root;
}

void StorageManager::setIndexOfRoot(uint32_t i) {
    assert(i > 0 && i < meta->count);
    meta->root = i;
}

bool StorageManager::initFile() {
    meta = (MetaBlock*)getBlock(0);
    if (meta == NULL) return false;

    meta->header.magic = 0xc1c6f01e;
    // init metadata block
    meta->header.type = BLOCK_TYPE_META;
    meta->header.index = 0;
    meta->header.next = 0;
    meta->root = 0;
    meta->free = 1;
    meta->count = NUM_BLOCK;
    meta->idle = NUM_BLOCK - 1;

    // free block list
    for (uint32_t i = 1; i < NUM_BLOCK; i++) {
        RecordBlock *block = (RecordBlock*)getBlock(i);
        if (block == NULL) return false;
        block->header.magic = 0xc1c6f01e;
        block->header.index = i;
        block->header.type = BLOCK_TYPE_FREE;
        // add to free lsit
        block->header.next = meta->free;
        meta->free = i;
    }

    return true;
    //return save();
}

////
// @file btree.h
// @breif
// B+tree
//
// @author yangtao
// @email yangtaojay@gamil.com
//




#pragma pack(1)

///
// @breif
// kv for B+tree
struct KeyValue {
    uint64_t key;
    uint32_t value;
    uint32_t position;
};

///
// @breif
// node of b+tree
struct NodeBlock {
    BlockHeader header;

    // The first key is empty, if the block is an interior node
#ifdef DEBUG_BTREE
    KeyValue kv[4];
#else
    KeyValue kv[1];
#endif // DEBUG_BTREE   

    static uint16_t size();

    bool full();

    bool empty();

    // equal
    bool eMin();

    // greater or equal
    // no need to maintain
    bool geMin();

    ///
    // @brief
    // return the index of first item whose key is not less than `key`
    // if keys of all items is less than `key`, return `header.count`
    uint16_t find(uint64_t key);

    void insert(KeyValue item, uint16_t index);

    void split(NodeBlock* nextNode);

    void merge(NodeBlock* nextNode);

    bool isLeaf();

    void remove(uint16_t index);
};

#pragma pack()

class BTree {
private:
    StorageManager &storage;
    NodeBlock* root;

    //void setRootIndex(uint32_t index);
    KeyValue insert(KeyValue kv, NodeBlock* cur);
    void remove(uint64_t key, NodeBlock* cur);
    void removeByMark(uint64_t key, NodeBlock* cur);
    KeyValue search(uint64_t key, NodeBlock* cur);

public:
    BTree(StorageManager &s);
    int insert(KeyValue kv);
    int remove(uint64_t key);
    KeyValue search(uint64_t key);
    std::vector<KeyValue> search(uint64_t lo, uint64_t hi);
    std::vector<KeyValue> lower(uint64_t lo);
    std::vector<KeyValue> upper(uint64_t hi);
};


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

void BTree::removeByMark(uint64_t key, NodeBlock * cur) {
    if (cur->empty()) {
        return;
    }
    uint16_t i = cur->find(key);
    if (cur->isLeaf()) {
        if (cur->kv[i].key == key)
            cur->kv[i].value = 0;
    }
    else {
        if (cur->kv[i].key > key || i == cur->header.count) i--;
        NodeBlock* s = (NodeBlock*)storage.getBlock(cur->kv[i].value);
        return removeByMark(key, s);
    }
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

    removeByMark(key, root);
    return 1;
}


KeyValue BTree::search(uint64_t key, NodeBlock* cur) {
    if (cur->empty()) {
        KeyValue res;
        res.value = 0;
        return res;
    }
    uint16_t i = cur->find(key);
    if (cur->isLeaf()) {
        if (cur->kv[i].key == key)
            return cur->kv[i];
    }
    else {
        if (cur->kv[i].key > key || i == cur->header.count) i--;
        NodeBlock* s = (NodeBlock*)storage.getBlock(cur->kv[i].value);
        return search(key, s);
    }
    KeyValue res;
    res.value = 0;
    return res;
}


KeyValue BTree::search(uint64_t key) {
    return search(key, root);
}

std::vector<KeyValue> BTree::search(uint64_t lo, uint64_t hi) {
    NodeBlock *cur = root;
    std::vector<KeyValue> res;
    if (root->empty()) return res;
    while (!cur->isLeaf()) {
        uint16_t i = cur->find(lo);
        if (cur->kv[i].key > lo || i == cur->header.count) i--;
        cur = (NodeBlock*)storage.getBlock(cur->kv[i].value);
    }
    while (cur->isLeaf()) {
        uint16_t i = cur->find(lo);
        for (; i < cur->header.count; i++) {
            if (cur->kv[i].key >= lo && cur->kv[i].key <= hi)
                res.push_back(cur->kv[i]);
        }
        if (cur->header.next == 0 || cur->kv[i].key > hi) break;
        cur = (NodeBlock*)storage.getBlock(cur->header.next);
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
        cur = (NodeBlock*)storage.getBlock(cur->kv[i].value);
    }
    while (cur->isLeaf()) {
        uint16_t i = cur->find(lo);
        for (; i < cur->header.count; i++) {
            if (cur->kv[i].key >= lo)
                res.push_back(cur->kv[i]);
        }
        if (cur->header.next == 0) break;
        cur = (NodeBlock*)storage.getBlock(cur->header.next);
    }
    return res;
}

std::vector<KeyValue> BTree::upper(uint64_t hi) {
    NodeBlock *cur = root;
    std::vector<KeyValue> res;
    if (root->empty()) return res;
    while (!cur->isLeaf()) {
        cur = (NodeBlock*)storage.getBlock(cur->kv[0].value);
    }
    while (cur->isLeaf()) {
        uint16_t i = 0;
        for (; i < cur->header.count; i++) {
            if (cur->kv[i].key <= hi)
                res.push_back(cur->kv[i]);
        }
        if (cur->header.next == 0 || cur->kv[i].key > hi) break;
        cur = (NodeBlock*)storage.getBlock(cur->header.next);
    }
    return res;
}







struct RowData {
    int id;
    bool sex;
    string name;
    string number;
    string email;
};
void initial();//初始化操作，创建数据库文件等
void insert(std::vector<RowData> rows);//插入数据,将rows数据插入到数据库中
std::vector<RowData> query(string sql); //查询数据,将满足要求的数据作为返回值返回。
void del(string sql);//删除数据
void update(string sql);//更新数据

Record* encode(const RowData &data) {
    uint16_t nameLen = data.name.size();
    uint16_t numLen = data.number.size();
    uint16_t emailLen = data.email.size();
    Record *r = (Record *)malloc(sizeof(RecordHeader) + 14 + nameLen + emailLen + numLen);
    r->header.size = 14 + nameLen + emailLen + numLen;
    uint8_t *p = (uint8_t*)r + sizeof(RecordHeader);
    *(uint16_t*)p = nameLen;
    p += 2;
    *(uint16_t*)p = numLen;
    p += 2;
    *(uint16_t*)p = emailLen;

    p += 2;
    *(int*)p = data.id;
    p += 4;
    *p = data.sex;
    p += 1;

    memcpy(p, data.name.c_str(), nameLen);
    p += nameLen;
    *p = '\0';
    p++;

    memcpy(p, data.number.c_str(), numLen);
    p += numLen;
    *p = '\0';
    p++;

    memcpy(p, data.email.c_str(), emailLen);
    p += emailLen;
    *p = '\0';
    return r;
}

RowData decode(const Record *record) {
    RowData data;
    const uint8_t *p = record->data;
    uint16_t nameLen = *(uint16_t*)p;
    uint16_t numLen = *(uint16_t*)(p + 2);
    uint16_t emailLen = *(uint16_t*)(p + 4);
    data.id = *(int*)(p + 6);
    data.sex = *(uint8_t*)(p + 10);

    data.name = (char*)(p + 11);

    data.number = (char*)(p + 12 + nameLen);

    data.email = (char*)(p + 13 + numLen + nameLen);
    return data;
}



static StorageManager s("table.db");
static StorageManager st("tree.db");
static BTree bTree(st);
static RecordBlock *block = NULL;
void initial() {}

void insert(std::vector<RowData> rows) {
    if (block == NULL) {
        if (s.getIndexOfRoot() == 0) {
            block = (RecordBlock *)s.getFreeBlock(NULL);
            block->init();
            s.setIndexOfRoot(block->header.index);
        }
        else
            block = (RecordBlock *)s.getBlock(s.getIndexOfRoot());
    }
    for (auto &r : rows) {
        if (bTree.search(r.id).value != 0) {
            continue;
        }
        uint32_t pos = 0;
        if (!block->addRecord(encode(r), &pos)) {
            RecordBlock *newBlock = (RecordBlock *)s.getFreeBlock(NULL);
            newBlock->init();

            block->header.next = newBlock->header.index;
            block = newBlock;

            s.setIndexOfRoot(block->header.index);

            if (!block->addRecord(encode(r), &pos)) {
                // TODO: 
            }
        }
        KeyValue kv;
        kv.key = r.id, kv.value = block->header.index, kv.position = pos;
        bTree.insert(kv);
    }
}

static int strToInt(string str) {
    int i = 0, res = 0;
    while (str[i]<'0' || str[i]>'9') i++;
    while (str[i] >= '0' && str[i] <= '9') {
        res = res * 10 + str[i] - '0';
        i++;
    }
    return res;
}

std::vector<RowData> query(string sql) {
    // select * from table where id >1500 and id < 1500;
    int i = 0;
    for (; i < sql.size(); i++) if (sql[i] == 'i' && sql[i + 1] == 'd') break;
    i++;
    int bigger = -2, eq = -2, less = -2;
    for (; i < sql.size(); i++) {
        switch (sql[i]) {
        case '<':
            bigger = strToInt(sql.substr(i + 1)) + (sql[i + 1] == '=' ? 1 : 0);
            break;
        case '=':
            if (sql[i - 1] != '<' && sql[i - 1] != '>')
                eq = strToInt(sql.substr(i + 1));
            break;
        case '>':
            less = strToInt(sql.substr(i + 1)) - (sql[i + 1] == '=' ? 1 : 0);
            break;
        default:
            break;
        }
    }
    vector<RowData> res;
    vector<KeyValue> r;
    if (sql.find("and") != string::npos) {
        r = bTree.search(less + 1, bigger - 1);
    }
    else if (sql.find("or") != string::npos) {
        r = bTree.lower(less + 1);
        auto t = bTree.upper(bigger - 1);
        for (auto &i : t) r.push_back(i);

    }
    else {
        if (eq != -2) {
            KeyValue kv = bTree.search(eq);
            if (kv.value != 0) r.push_back(kv);
        }
        if (less != -2) r = bTree.lower(less + 1);;
        if (bigger != -2) r = bTree.upper(bigger - 1);
    }
    for (auto & i : r) {
        if (i.value == 0) continue;
        RecordBlock *rb = (RecordBlock*)s.getBlock(i.value);
        auto t = decode(rb->getRecord(i.position));
        res.push_back(t);
    }
    return res;
}

void del(string sql) {
    //delete from table where id > 300 or id > 784
    int i = 0;
    for (; i < sql.size(); i++) if (sql[i] == 'i' && sql[i + 1] == 'd') break;
    i++;
    int bigger = -2, eq = -2, less = -2;
    for (; i < sql.size(); i++) {
        switch (sql[i]) {
        case '<':
            bigger = strToInt(sql.substr(i + 1)) + (sql[i + 1] == '=' ? 1 : 0);
            break;
        case '=':
            if (sql[i - 1] != '<' && sql[i - 1] != '>')
                eq = strToInt(sql.substr(i + 1));
            break;
        case '>':
            less = strToInt(sql.substr(i + 1)) - (sql[i + 1] == '=' ? 1 : 0);
            break;
        default:
            break;
        }
    }

    vector<KeyValue> r;
    if (sql.find("and") != string::npos) {
        r = bTree.search(less + 1, bigger - 1);
    }
    else if (sql.find("or") != string::npos) {
        r = bTree.lower(less + 1);
        auto t = bTree.upper(bigger - 1);
        for (auto &i : t) r.push_back(i);

    }
    else {
        if (eq != -2) {
            KeyValue kv = bTree.search(eq);
            if (kv.value != 0) r.push_back(kv);
        }
        if (less != -2) r = bTree.lower(less + 1);;
        if (bigger != -2) r = bTree.upper(bigger - 1);
    }

    for (auto & i : r) {

        //RecordBlock *rb = (RecordBlock*)s.getBlock(i.value);
        //rb->delRecord(i.position);
        if (i.value != 0)
            bTree.remove(i.key);
    }
}

void update(string sql) {
    //update table set number = 20157894 where id<900 or id>9950
    int i = 0;
    for (; i < sql.size(); i++) if (sql[i] == 'i' && sql[i + 1] == 'd') break;
    i++;
    int bigger = -2, eq = -2, less = -2;
    for (; i < sql.size(); i++) {
        switch (sql[i]) {
        case '<':
            bigger = strToInt(sql.substr(i + 1)) + (sql[i + 1] == '=' ? 1 : 0);
            break;
        case '=':
            if (sql[i - 1] != '<' && sql[i - 1] != '>')
                eq = strToInt(sql.substr(i + 1));
            break;
        case '>':
            less = strToInt(sql.substr(i + 1)) - (sql[i + 1] == '=' ? 1 : 0);
            break;
        default:
            break;
        }
    }

    int sex = -1;
    string num = "", mail = "", name = "";

    for (i = 0; i < sql.size(); i++) if (sql[i] == 's' && sql[i + 1] == 'e') break;
    i += 3;
    while (sql[i] == ' ')i++;
    if (sql[i] == 's') {
        //sex
        while (sql[i] != '=') i++;
        i++;
        sex = strToInt(sql.substr(i));
    }
    else if (sql[i] == 'e') {
        //email
        while (sql[i] != '=') i++;
        i++;
        while (sql[i] == ' ')i++;
        int j = i + 1;
        while (sql[j] != ' ') j++;
        mail = sql.substr(i, j - i);
    }
    else if (sql[i] == 'n' && sql[i + 1] == 'a') {
        //name
        while (sql[i] != '=') i++;
        i++;
        while (sql[i] == ' ')i++;
        int j = i + 1;
        while (sql[j] != ' ') j++;
        name = sql.substr(i, j - i);
    }
    else if (sql[i] == 'n' && sql[i + 1] == 'u') {
        //number
        while (sql[i] != '=') i++;
        i++;
        while (sql[i] == ' ')i++;
        int j = i + 1;
        while (sql[j] != ' ') j++;
        num = sql.substr(i, j - i);
    }


    vector<KeyValue> r;
    if (sql.find("and") != string::npos) {
        r = bTree.search(less + 1, bigger - 1);
    }
    else if (sql.find("or") != string::npos) {
        r = bTree.lower(less + 1);
        auto t = bTree.upper(bigger - 1);
        for (auto &i : t) r.push_back(i);

    }
    else {
        if (eq != -2) {
            KeyValue kv = bTree.search(eq);
            if (kv.value != 0) r.push_back(kv);
        }
        if (less != -2) r = bTree.lower(less + 1);;
        if (bigger != -2) r = bTree.upper(bigger - 1);
    }

    for (auto &i : r) {
        if (i.value == 0) continue;
        RecordBlock *rb = (RecordBlock*)s.getBlock(i.value);
        auto t = decode(rb->getRecord(i.position));

        if (sex != -1) {
            t.sex = sex;
        }
        if (name != "") {
            t.name = name;
        }
        if (num != "") {
            t.number = num;
        }
        if (mail != "") {
            t.email = mail;
        }

        rb->updateRecord(i.position, encode(t));
    }
}


void showRowData(const RowData &data) {
    std::cout << data.id << "\t" << data.sex << "\t" << data.name << "\t" << data.number << "\t" << data.email << std::endl;
}

