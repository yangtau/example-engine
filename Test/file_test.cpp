

#include "CppUnitTest.h"
#include "stdafx.h"
#include "../Project/file.h"
#include "../Project/buffer.h"
#include <string>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest {
    TEST_CLASS(FileUnitTest) {
public:

    TEST_METHOD(FileCreate) {
        File file;
        int res = file.create("file-test.db", 20);
        Assert::AreNotEqual(0, res);
    }

    TEST_METHOD(FileReadBlock) {
        File file;
        int res = file.create("file-test.db", 20);
        BufferManager buffer;
        Assert::AreNotEqual(0, res);
        

        RecordBlock *block = (RecordBlock *)buffer.allocateBlock();
        Assert::AreEqual(true, file.readBlock(3, block));
        buffer.freeBlock(block);

    }

    TEST_METHOD(FileWriteBlock) {
        File file;
        int res = file.create("file-test.db", 20);
        Assert::AreNotEqual(0, res);
        BufferManager buffer;
        RecordBlock *block = (RecordBlock *)buffer.allocateBlock();
        Assert::AreEqual(true, file.readBlock(3, block));

        // write to block
        char data[10] = { 6, 0, 0, 0, 'H', 'e', 'l', 'l', 'o', '\0' };
        memcpy((char *)block + sizeof(BlockHeader), data, 10);
        Assert::AreEqual(true, file.writeBlock(3, block));

        Assert::AreEqual(true, file.readBlock(3, block));
        Assert::AreEqual(std::string(data + 4), std::string((char*)block + sizeof(BlockHeader) + 4));
    }

    };
}


