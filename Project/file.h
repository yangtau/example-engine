////
// @file file.h
// @brief
// file
//
// @author yangtao
// @email yangtaojay@gmail.com
//

#pragma once

#include "Windows.h"
#include "block.h"
#include "inttypes.h"

class File {
private:
	HANDLE _handle;
	uint8_t *buffer;
	uint8_t *headerBuffer;

public:
	File() : _handle(INVALID_HANDLE_VALUE), buffer(NULL) {}

	~File() {
		if (buffer != NULL)
			_aligned_free(buffer);
		if (_handle != INVALID_HANDLE_VALUE)
			CloseHandle(_handle);
	}

	// the number of blocks
	int create(char *path, uint32_t num);

	// the index of block is the order the block in the file, starting from 0
	Block *allocateBlock(uint32_t index);

	int writeBlock(uint32_t index, Block *block);
};

struct FileHeader {
	uint32_t free;
	uint32_t used;
};