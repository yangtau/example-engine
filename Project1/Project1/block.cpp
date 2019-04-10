#include "block.h"

uint16_t Header::compute() {
	// the max sum 4094/2 * 2^16 < 2^32
	uint32_t sum = 0;
	// Size of block is even, so there is no need to handle odd-sized block
	for (int i = 0; i < BLOCK_SIZE / 2; i++) {
		sum += ((uint16_t *)this)[i]; // get two bytes
	}
	sum -= checksum; // substract checksum from sum
	// Fold to get ones-complement result
	while (sum >> 16) {
		sum = (sum & 0xffff) + (sum >> 16);
	}
	return ~sum;
}

int Header::check() {
	uint32_t sum = 0;
	// skip check sum, start with i=2
	for (int i = 0; i < BLOCK_SIZE / 2; i++) {
		sum += ((uint16_t *)this)[i]; // get two bytes
	}
	while (sum >> 16) {
		sum = (sum & 0xffff) + (sum >> 16);
	}
	return sum == 0xffff;
}

Tailer *Block::getTailer() {

	return nullptr;
}
