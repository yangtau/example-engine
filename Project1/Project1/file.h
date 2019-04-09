////
// @file file.h
// @brief
// 存储文件
//
// @author yangtao
// @email yangtaojay@gmail.com
//
#pragma once

#include <Windows.h>
#include <string>

////
// @brief
// 管理文件
//
class File {
private:
  HANDLE handle_; // 文件句柄

public:
  File() : handle_(INVALID_HANDLE_VALUE) {}
  int create(std::string path, int size);
};
