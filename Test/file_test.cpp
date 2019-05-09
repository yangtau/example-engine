

#include "CppUnitTest.h"
#include "stdafx.h"
#include "../Project/file.h"
#include <string>
#include <cstring>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest {
    TEST_CLASS(FileUnitTest) {
public:

    TEST_METHOD(FileCreate) {
        File file;
        int res = file.create("hello.db", 20);
        Assert::AreEqual(res, 1);
    }

    TEST_METHOD(FileAllocateBlock) {
        File file;
        int res = file.create("world.db", 20);
        Assert::AreEqual(1, res);
        {
            RecordBlock *block = (RecordBlock *)file.getBlock(3);
            int re = block != NULL;
            Assert::AreEqual(1, re);
        }
        {
            RecordBlock *block = (RecordBlock *)file.getBlock(20);
            int re = block != NULL;
            Assert::AreEqual(0, re);
        }
    }

    TEST_METHOD(FileWriteBlock) {
        File file;
        int res = file.create("helloworld.db", 20);
        Assert::AreEqual(1, res);
        RecordBlock *block = (RecordBlock *)file.getBlock(3);

        // write to block
        char data[10] = { 6, 0, 0, 0, 'H', 'e', 'l', 'l', 'o', '\0' };
        memcpy((char *)block + sizeof(BlockHeader), data, 10);
        file.writeBlock(3, block);

        block = (RecordBlock *)file.getBlock(3);
        Assert::AreEqual(std::string(data + 4), std::string((char*)block + sizeof(BlockHeader)+4));
    }

    };
}


