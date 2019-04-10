////
// @file file.h
// @brief
// block
//
// @author yangtao
// @email yangtaojay@gmail.com
//

#pragma once
#include <inttypes.h>

#pragma pack(1)
struct Header {
  // uint16_t type;     // 块类型
  uint16_t checksum; // Tcp头部校验
  uint16_t count;    // 记录数目
  uint16_t free;     // 空闲块
  uint32_t next;     // 下一个block的文件偏移量

  uint16_t compute(); // 计算checksum
  int check();        // check checksum
};

struct Tailer {
  uint16_t slots[1]; // 向上增长的记录偏移量数组
};

#pragma pack()

const uint32_t BLOCK_SIZE = 4096; // 全局静态变量

struct Block {
  Header header; // 头部

  Tailer *getTailer();
  void addRecord(void *buffer, int size);
  void delRecord(void *buffer);
  void updateRecord();
};
