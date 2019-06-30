////
// @file file.h
// @brief
// file
//
// @author yangtao
// @email yangtaojay@gmail.com
//

#pragma once

#include "block.h"
#include <Windows.h>

///
// @brief
// File class
class MyFile {
private:
  HANDLE handle;

public:
  MyFile() : handle(INVALID_HANDLE_VALUE) {}

  ~MyFile() {
    if (handle != INVALID_HANDLE_VALUE)
      close();
  }

  // @brief
  // create a file with initial size
  // return 1 if every thing is ok
  int create(const char *path, u32 numberOfBlock, u32 blockSize);

  int open(const char *path);

  void close() {
    if (handle != INVALID_HANDLE_VALUE)
      CloseHandle(handle);
  };

  // @brief
  // the real position is offset * size from the begin of file
  int readBlock(void *block, u32 offset, u32 blockSize);

  int writeBlock(const void *block, u32 offset, u32 blockSize);

  int resize(u32 numberOfBlock, u32 blockSize);
};
