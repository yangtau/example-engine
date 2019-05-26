////
// @file buffer.h
// @brief
// buffer manager
// 
// @author yangtao
// @email yangtaojay@gmail.com
// 

#pragma once

class BufferManager {

public:
    void *allocateBlock();
    void freeBlock(void*);
};

