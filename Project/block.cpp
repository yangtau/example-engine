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

void RecordBlock::init() {
    count = 0;
    free = BLOCK_SIZE;
    header.magic = MAGIC_NUM;
    header.type = BLOCK_TYPE_DATA;
}

int RecordBlock::freeSize() {
    return (int)free - ((uint8_t*)&directory[count + 1] - (uint8_t*)this);
}

int RecordBlock::addRecord(Record* record, uint32_t *position) {
    if (record->size > freeSize()) {
        return false;
    }
    RecordBlock::free -= record->size;

    memcpy((uint8_t*)this + free, record, record->size);

    if (position != NULL) *position = RecordBlock::count;

    RecordBlock::directory[RecordBlock::count++] = RecordBlock::free;
    return true;
}

int RecordBlock::delRecord(uint32_t position) {
    if (position >= count) {
        return false;
    }

    // a simple implementation
    directory[position] = 0;

    //uint16_t offset = directory[position];
    //
    //if (offset == 0) return false;

    //Record* record = (Record*)((uint8_t*)this + offset);
    //uint16_t size = record->size;
    //directory[position] = 0;  // pointer to 0
    //// move memory, `offset + size - header.free` is the size to move
    //memmove(record, (uint8_t*)record + size, header.free - offset + size);
    //header.free -= size;
    //// modify offset table (tailer)
    //for (uint16_t i = 0; i < header.count; i++) {
    //    if (getTailer()->slots[i] > offset) {
    //        getTailer()->slots[i] -= size;
    //    }
    //}
    return true;
}

Record* RecordBlock::getRecord(uint32_t position) {
    if (position >= count) {
        // todo
        return NULL;
    }

    uint16_t offset = directory[position];

    // if the record is removed, the offset is 0
    if (offset == 0) return NULL;

    Record* res = (Record*)((uint8_t*)this + offset);
    // res->data = (uint8_t*)this + offset + sizeof(RecordHeader);

    return res;
}

int RecordBlock::updateRecord(uint32_t position, Record* record) {
    Record* old = getRecord(position);  // get the old record
    if (old == NULL) {
        // the position of the old record is false
        return false;
    }

    if (old->size >= record->size) {
        //old->header = record->header;
        memcpy((uint8_t*)old, record, record->size);
        return true;
    }

    if (record->size > freeSize()) {
        // memory is not enough, add to next block
        return false;
    }

    delRecord(position);

    // insert
    RecordBlock::free -= record->size;

    memcpy((uint8_t*)this + free, record, record->size);

    RecordBlock::directory[position] = RecordBlock::free;
    return true;

    //uint16_t offset = getTailer()->slots[position];
    //if (offset == 0) return false;

    //uint16_t size = old->size;
    //// move memory, `offset + size - header.free` is the size to move
    //memmove(old, (uint8_t*)old + size, header.free - offset - size);
    //header.free -= size;
    //// modify offset table (tailer)
    //for (uint16_t i = 0; i < header.count; i++) {
    //    if (getTailer()->slots[i] > offset) {
    //        getTailer()->slots[i] -= size;
    //    }
    //}
    ////memcpy((uint8_t*)this + header.free, record, sizeof(RecordHeader));
    //// *(RecordHeader*)((uint8_t*)this + header.free) = record->header;
    //// copy data of record
    //memcpy((uint8_t*)this + header.free,
    //    record->data, record->size);
    //getTailer()->slots[position] = header.free;  // point to new position
    //header.free += record->size;
    return true;
}

