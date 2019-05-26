#include "CppUnitTest.h"
#include "stdafx.h"
#include <string>
#include "../Project/storage.h"


using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest {
    TEST_CLASS(StorageUnitTest) {
public:
    TEST_METHOD(getFreeBlock) {
        StorageManager s = StorageManager("storage-freeblock.db");
        RecordBlock *block = (RecordBlock*)s.getFreeBlock();
        Assert::IsNotNull(block);
        Assert::AreEqual(block->header.index, 1u);

        block = (RecordBlock*)s.getFreeBlock();
        Assert::IsNotNull(block);
        Assert::AreEqual(block->header.index, 2u);

        s.freeBlock(block->header.index);

        block = (RecordBlock*)s.getFreeBlock();
        Assert::IsNotNull(block);
        Assert::AreEqual(block->header.index, 2u);

        block = (RecordBlock*)s.getFreeBlock();
        Assert::IsNotNull(block);
        Assert::AreEqual(block->header.index, 3u);
    }

    TEST_METHOD(getBlock) {
        StorageManager s = StorageManager("storage-test.db");

        Assert::IsNotNull(s.readBlock(1));

    }

    TEST_METHOD(resize) {
        StorageManager s = StorageManager("storage-test.db");
        for (int i = 0; i < 2 * 64; i++)
            Assert::IsNotNull(s.getFreeBlock());
    }

    TEST_METHOD(save) {
        StorageManager s = StorageManager("storage-test.db");
        //uint32_t index = 0;
        RecordBlock *block = (RecordBlock *)s.getFreeBlock();
        Assert::IsNotNull(block);

        char data[10] = { 6, 0, 0, 0, 'H', 'e', 'l', 'l', 'o', '\0' };
        memcpy((char *)block + sizeof(BlockHeader), data, 10);

        Assert::AreEqual(true, s.save());
    }

    TEST_METHOD(indexOfRoot) {
        StorageManager s = StorageManager("storage-test.db");
        uint32_t index = 20;

        s.setIndexOfRoot(index);

        Assert::AreEqual(index, s.getIndexOfRoot());
    }
    };
}
