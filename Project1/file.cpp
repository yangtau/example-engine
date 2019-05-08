////
// @file file.cpp
// @brief
// block
//
// @author yangtao
// @email yangtaojay@gmail.com
//


#include "file.h"

int File::create(char *path, uint32_t num) {
	_handle = CreateFileA((TCHAR *)path, GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
		FILE_FLAG_NO_BUFFERING, nullptr);

	headerBuffer = (uint8_t *)_aligned_malloc(BLOCK_SIZE, BLOCK_SIZE);
	if (headerBuffer == NULL) return false;
	else VirtualLock(headerBuffer, BLOCK_SIZE);

	if (_handle == INVALID_HANDLE_VALUE) {
		// the file does not exist
		_handle = CreateFileA((TCHAR *)path, GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_DELETE, nullptr, CREATE_ALWAYS,
			FILE_FLAG_NO_BUFFERING, nullptr);
		// set the size of file
		BOOL res = SetFilePointer(_handle, BLOCK_SIZE*num, NULL, FILE_BEGIN)
			&& SetEndOfFile(_handle);
		if (!res) return false;
		// init file
		unsigned long dwBytesToWrite = -1;
		// free block
		Block *block = (Block*)headerBuffer;
		block->header.count = 0, block->header.free = sizeof(BlockHeader);
		for (uint32_t i = 1; i < num; i++) {
			block->header.next = i + 1;
			block->header.compute();
			if (SetFilePointer(_handle, BLOCK_SIZE*i, NULL, FILE_BEGIN) != BLOCK_SIZE * i)
				return false;
			if (!WriteFile(_handle, headerBuffer, BLOCK_SIZE, &dwBytesToWrite, NULL)
				&& dwBytesToWrite != BLOCK_SIZE)
				return false;
		}
		// first block
		FileHeader *header = (FileHeader*)headerBuffer;
		header->free = 1, header->used = 0;
		if (SetFilePointer(_handle, 0, NULL, FILE_BEGIN) != 0)
			return false;
		if (!WriteFile(_handle, headerBuffer, BLOCK_SIZE, &dwBytesToWrite, NULL)
			&& dwBytesToWrite != BLOCK_SIZE)
			return false;
	}
	else {
		if (SetFilePointer(_handle, 0, NULL, FILE_BEGIN) != 0)
			return false;
		if (!ReadFile(_handle, headerBuffer, BLOCK_SIZE, NULL, NULL))
			return false;
	}
	return _handle != INVALID_HANDLE_VALUE;
}



Block *File::allocateBlock(uint32_t index) {
	// allocate buffer
	if (buffer == nullptr) {
		buffer = (uint8_t *)_aligned_malloc(BLOCK_SIZE, BLOCK_SIZE);
		if (buffer == nullptr) return NULL;
		else VirtualLock(buffer, BLOCK_SIZE);
	}
	unsigned long bytesToRead = -1;
	uint64_t offset = index * BLOCK_SIZE;  // the position of the block
	DWORD fp = SetFilePointer(_handle, offset, NULL,
		FILE_BEGIN  // move from the begin of file
	);
	if (fp != offset)
		return NULL;
	if (!ReadFile(_handle, buffer, BLOCK_SIZE, &bytesToRead, NULL))
		return NULL;
	return (Block *)buffer;
}

int File::writeBlock(uint32_t index, Block * block) {
	unsigned long bytesToWrite = -1;
	uint64_t offset = index * BLOCK_SIZE;  // the position of the block
	DWORD fp = SetFilePointer(_handle, offset, NULL, FILE_BEGIN);
	if (fp != offset) return 0;
	if (!WriteFile(_handle, block, BLOCK_SIZE, &bytesToWrite, NULL)
		&& bytesToWrite == BLOCK_SIZE)
		return 0;
	return 1;
}
