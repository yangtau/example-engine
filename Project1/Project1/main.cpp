#include <iostream>
#include "block.h"

uint8_t *buffer = (uint8_t*)malloc(BLOCK_SIZE);
Block *block = (Block*)buffer;
uint8_t data[10] = { 6, 0, 0,0, 0, 1, 2, 4, 5, 3 };
unsigned size = 6;

int main() {
	block->header.count = 0;
	block->header.free = sizeof(BlockHeader);
	Record *record = (Record*)data;
	int res = block->addRecord(record);
	const Record * rec = block->getRecord(0);

	uint8_t *r = record->getData();

	getchar();
	return 0;
}