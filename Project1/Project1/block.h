////
// @file file.h
// @brief
// block
//
// @author yangtao
// @email yangtaojay@gmail.com
//

#pragma once

#pragma pack(1)
struct Header {
  unsigned short type;     // 块类型
  unsigned short checksum; // Tcp头部校验
  unsigned short count;    // 记录数目
  unsigned short free;     // 空闲块
  unsigned int next;       // 下一个block的文件偏移量

  unsigned short compute(); // 计算checksum
};

struct Tailer {
  unsigned short directory[1]; // 向上增长的记录偏移量数组
};
#pragma pack()

struct Block {
  static unsigned int size; // 全局静态变量
  Header header;            // 头部

  Tailer *getTailer();
  void addRecord(void *buffer, int size);
};
