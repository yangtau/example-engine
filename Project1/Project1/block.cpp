#include "block.h"
#include <cstring>

uint8_t* Record::getData() {
	return (uint8_t*)this + sizeof(RecordHeader);
}

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
	// skip check sum, start with i=2
	for (int i = 0; i < BLOCK_SIZE / 2; i++) {
		sum += ((uint16_t*)this)[i];  // get two bytes
	}
	while (sum >> 16) {
		sum = (sum & 0xffff) + (sum >> 16);
	}
	return sum == 0xffff;
}

Tailer* Block::getTailer() {
	// `BLOCK_SIZE - 2 * header.count` is offset of the tailor
	return (Tailer*)((uint8_t*)(this) + BLOCK_SIZE - 2 * header.count);
}

int Block::delRecord(uint32_t position) {
	if (position > header.count) {
		// todo
		// find in next block
		return false;
	}

	uint16_t offset = getTailer()->slots[position];
	Record* record = (Record*)((uint8_t*)this + offset);
	uint16_t size = record->header.size + sizeof(RecordHeader);
	getTailer()->slots[position] = 0;  // pointer to 0
	// move memory, `offset + size - header.free` is the size to move
	memmove(record, (uint8_t*)record + size, offset + size - header.free);
	header.free -= size;
	// modify offset table (tailer)
	for (uint16_t i = 0; i < header.count; i++) {
		if (getTailer()->slots[i] > offset) {
			getTailer()->slots[i] -= size;
		}
	}
	return true;
}

int Block::addRecord(Record* record) {
	uint16_t size = record->header.size + sizeof(RecordHeader);
	// `BLOCK_SIZE - 2 * header.count` is offset of the tailor, 2 bytes for
	// offset
	if (size + 2 + header.free >= BLOCK_SIZE - 2 * header.count) {
		// todo
		// memory is not enough, add to next block
		return false;
	}
	// copy header record
	memcpy((uint8_t*)this + header.free, record, sizeof(RecordHeader));
	// *(RecordHeader*)((uint8_t*)this + header.free) = record->header;
	// copy data of record
	memcpy((uint8_t*)this + header.free + sizeof(RecordHeader),
		record->getData(), record->header.size);
	// order of following operations is important !!!
	header.count++;
	getTailer()->slots[0] = header.free;  // getTailer() is calculated by count,
										  // so count must be update before this
	header.free += size;
	return true;
}

Record* Block::getRecord(uint32_t position) {
	if (position > header.count) {
		// todo
		// find in next block
		return NULL;
	}
	uint16_t offset = getTailer()->slots[position];
	// if the record is removed, the offset is 0
	if (offset == 0) return NULL;
	Record* res = (Record*)((uint8_t*)this + offset);
	// res->data = (uint8_t*)this + offset + sizeof(RecordHeader);
	return res;
}

int Block::updateRecord(uint32_t position, Record* record) {
	Record* old = getRecord(position);  // get the old record
	old->header = record->header;
	memcpy((uint8_t*)old + sizeof(RecordHeader), record->getData(),
		record->header.size);
	return true;
}