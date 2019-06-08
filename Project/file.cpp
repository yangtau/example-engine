////
// @file file.cpp
// @brief
// block
//
// @author yangtao
// @email yangtaojay@gmail.com
//

#include <assert.h>
#include "file.h"


bool EXFile::readBlock(uint32_t index, void *block) {
    assert(block != NULL);

    // offset
    uint64_t offset = index * BLOCK_SIZE;  // the position of the block
    if (SetFilePointer(handle, offset, NULL, FILE_BEGIN) != offset)
        return false;

    // read
    unsigned long bytesToRead = -1;
    if (!ReadFile(handle, block, BLOCK_SIZE, &bytesToRead, NULL))
        return false;

    return true;
}

int EXFile::create(const char* path, uint32_t num) {
    // open file
    handle = CreateFileA((LPCSTR)path, GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        //FILE_ATTRIBUTE_NORMAL,
        FILE_FLAG_NO_BUFFERING,
        nullptr);
    // read metadata block
//    if (handle != INVALID_HANDLE_VALUE)
//        meta = (MetaBlock *) readBlock(META_INDEX);


    if (handle == INVALID_HANDLE_VALUE) {
        // the file does not exist
        // create file
        handle = CreateFileA((LPCSTR)path,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_DELETE,
            nullptr,
            CREATE_ALWAYS,
            //FILE_ATTRIBUTE_NORMAL,
            FILE_FLAG_NO_BUFFERING,
            nullptr);
        if (handle == INVALID_HANDLE_VALUE)
            return 0;

        // set the size of file, `BLOCK_SIZE * num` is the size of file
        BOOL res = SetFilePointer(handle, BLOCK_SIZE * num, NULL, FILE_BEGIN) &&
            SetEndOfFile(handle);
        if (!res)
            return 0;

        return 1;
    }
    else return 2;

}

//void *File::getBlock(uint32_t index) {
//    if (meta == NULL || index >= meta->count) return NULL;
//
//    RecordBlock *block = (RecordBlock *) readBlock(index);
//
//    if (!(block->header.check())) {
//        // TODO: wrong checksum
//    }
//    return block;
//}

//void *File::getFreeBlock(uint32_t *index) {
//    // TODO: alloc more block
//    if (meta->idle == 0 || meta->free == 0)
//        return NULL;
//
//    RecordBlock *b = (RecordBlock *) getBlock(meta->free);
//    if (b == NULL) return NULL;
//
//    if (index != NULL) *index = meta->free;
//    // remove from free list
//    meta->free = b->header.next;
//
//    meta->idle--;
//
//    return b;
//}

bool EXFile::writeBlock(uint32_t index, void *_block) {
    RecordBlock *block = (RecordBlock *)_block;
    // offset
    uint64_t offset = index * BLOCK_SIZE;  // the position of the block

    if (SetFilePointer(handle, offset, NULL, FILE_BEGIN) != offset)
        return false;

    // compute checksum
    block->header.compute();

    // write
    unsigned long bytesToWrite = -1;
    return WriteFile(handle, block, BLOCK_SIZE, &bytesToWrite, NULL) &&
        bytesToWrite == BLOCK_SIZE;
    // free block buffer
}

bool EXFile::resize(uint32_t num) {
    return SetFilePointer(handle, BLOCK_SIZE * num, NULL, FILE_BEGIN) &&
        SetEndOfFile(handle);
}

