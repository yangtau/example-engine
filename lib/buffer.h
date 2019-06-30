////
// @file storage.h
// @brief
// storage manager
//
// @author yangtao
// @email yangtaojay@gmail.com
//

#pragma once

#include <unordered_map>
#include "block.h"  // BlockHeader
#include "common.h"
#include "file.h"

///
// @brief
// buffer manager
// map to one file
//
class BufferManager {
 private:
  MyFile *file;
  const u32 blockSize;

  std::unordered_map<u32, BlockHeader *> pool;  // buffer pool

 public:
  BufferManager(MyFile *file, u32 blockSize);

  ~BufferManager();

  // @brief
  // if flag is 1, the data in block may be modified.
  // flag 0: read only
  void *getBlock(u32 index, int flag = 1);

  int save();
};