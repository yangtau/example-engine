#include "CppUnitTest.h"
#include "stdafx.h"
#include <string>
#include "../Project/storage.h"


using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest {
    TEST_CLASS(StorageUnitTest) {
public:
    TEST_METHOD(getFreeBlock) {
        BufferManager s;
        s.create("storage-freeblock.db");
        s.open("storage-freeblock.db");
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
        BufferManager s;
        s.create("storage-test.db");
        s.open("storage-test.db");
        Assert::IsNotNull(s.getBlock(0));

    }

    TEST_METHOD(resize) {
        BufferManager s;
        s.create("storage-test.db");
        s.open("storage-test.db");
        for (int i = 0; i < 2 * 64; i++)
            Assert::IsNotNull(s.getFreeBlock());
    }

    TEST_METHOD(save) {
        BufferManager s;
        s.create("storage-test.db");
        s.open("storage-test.db");
        //u32 index = 0;
        RecordBlock *block = (RecordBlock *)s.getFreeBlock();
        Assert::IsNotNull(block);

        u32 index = block->header.index;

        char data[10] = { 8, 0, 'H', 'e', 'l', 'l', 'o', '\0' };

        u16 position;
        block->init();
        Assert::AreEqual(1, block->addRecord((Record*)data, &position));

        Assert::AreEqual(true, s.save());
        s.close();

        s.open("storage-test.db");
        block = (RecordBlock*)s.getBlock(index);

        Record *r = block->getRecord(position);
        Assert::AreEqual(std::string(data + 2), std::string((char*)r->data));
    }

    TEST_METHOD(indexOfRoot) {
        BufferManager s;
        s.create("storage-test.db");
        s.open("storage-test.db");

        u32 index = 20;

        s.setIndexOfRoot(index);

        Assert::AreEqual(index, s.getIndexOfRoot());
    }
    };
}
