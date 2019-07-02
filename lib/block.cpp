////
// @file block.cpp
// @brief
// block
//
// @author yangtao
// @email yangtaojay@gmail.com
//

#include "block.h"
#include <cstring>
#include "debug.h"

u16 BlockHeader::compute() {
    // the max sum 4094/2 * 2^16 < 2^32
    u32 sum = 0;
    // Size of block is even, so there is no need to handle odd-sized block
    for (int i = 0; i < BLOCK_SIZE / 2; i++) {
        sum += ((u16*)this)[i];  // get two bytes
    }
    sum -= this->checksum;  // substract checksum from sum
    // Fold to get ones-complement result
    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    return checksum = ~sum;
}

int BlockHeader::check() {
    u32 sum = 0;
    for (int i = 0; i < BLOCK_SIZE / 2; i++) {
        sum += ((u16*)this)[i];  // get two bytes
    }
    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    return sum == 0xffff;
}

void DataBlock::init(u32 index) {
    header.init(index);
    count = 0;
    free = BLOCK_SIZE;
    header.type = BLOCK_TYPE_DATA;
}

int DataBlock::freeSize() {
    return (int)free - ((u8*)&directory[count + 1] - (u8*)this) - sizeof(u16) -
           2;  // TODO: two more bytes
}

int DataBlock::insert(const void* data, u16 size, u16* position) {
    MEM_CHECK
    if (size > freeSize()) {
        return false;
    }
    // TODO: add two more byte
    free -= size + 2;

    memcpy((u8*)this + free, data, size);
    if (position != NULL)
        *position = count;

    directory[count++] = free;
    MEM_CHECK
    return true;
}

const void* DataBlock::get(u16 position) {
    if (position >= count) {
        return NULL;
    }

    u16 offset = directory[position];

    //assert(offset < 4096);
    // if the record is removed, the offset is 0
    if (offset == 0)
        return NULL;

    return ((u8*)this + offset);
}

//
// int RecordBlock::del(u16 position) {
//    if (position >= count) {
//        return false;
//    }
//
//    // a simple implementation
//    directory[position] = 0;
//
//    return true;
//}
//
// int RecordBlock::update(u16 position, const Record *record) {
//    Record *old = get(position);  // get the old record
//    if (old == NULL) {
//        // the position of the old record is false
//        return false;
//    }
//
//    if (old->size >= record->size) {
//        //old->header = record->header;
//        memcpy((u8 *) old, record, record->size);
//        return true;
//    }
//
//    if (record->size > freeSize()) {
//        // memory is not enough, add to next block
//        return false;
//    }
//
//    del(position);
//
//    // insert
//    RecordBlock::free -= record->size;
//
//    memcpy((u8 *) this + free, record, record->size);
//
//    RecordBlock::directory[position] = RecordBlock::free;
//    return true;
//
//}
