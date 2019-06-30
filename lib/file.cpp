////
// @file file.cpp
// @brief
// block
//
// @author yangtao
// @email yangtaojay@gmail.com
//

#include "file.h"
#include <Windows.h>
#include <assert.h>

int MyFile::create(const char *path, u32 numberOfBlock, u32 blockSize) {
  // create file
  handle = CreateFileA((LPCSTR)path, GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL, CREATE_ALWAYS,
                       FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH, NULL);
  if (handle == INVALID_HANDLE_VALUE) return 0;

  if (!(SetFilePointer(handle, blockSize * numberOfBlock, NULL, FILE_BEGIN) &&
        SetEndOfFile(handle)))
    return 0;
  close();
  return 1;
}

int MyFile::open(const char *path) {
  handle = CreateFileA((LPCSTR)path, GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_DELETE | FILE_SHARE_READ, NULL, OPEN_EXISTING,
                       FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH, NULL);
  if (handle == INVALID_HANDLE_VALUE) return 0;
  return 1;
}

int MyFile::writeBlock(const void *block, u32 offset, u32 blockSize) {
  // the position of the block
  uint64_t off = offset * blockSize;

  if (SetFilePointer(handle, off, NULL, FILE_BEGIN) != off) return false;
  // write
  unsigned long bytesToWrite = -1;

  // FlushFileBuffers(handle);

  return WriteFile(handle, block, BLOCK_SIZE, &bytesToWrite, NULL) &&
         bytesToWrite == BLOCK_SIZE;
  // free block buffer
}

int MyFile::readBlock(void *block, u32 offset, u32 blockSize) {
  if (block == NULL) return 0;
  // the position of the block
  uint64_t off = offset * blockSize;

  if (SetFilePointer(handle, off, NULL, FILE_BEGIN) != off) return false;
  // read
  unsigned long bytesToRead = -1;
  return ReadFile(handle, block, BLOCK_SIZE, &bytesToRead, NULL);
}

int MyFile::resize(u32 numberOfBlock, u32 blockSize) {
  return SetFilePointer(handle, blockSize * numberOfBlock, NULL, FILE_BEGIN) &&
         SetEndOfFile(handle);
}
