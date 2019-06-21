#include "CppUnitTest.h"
#include "stdafx.h"
#include "../Project/block.h"
#include <string>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest {
    TEST_CLASS(BlockUnitTest) {
public:


    TEST_METHOD(BlockCheck) {
        short *buffer = (short*)malloc(BLOCK_SIZE);
        for (int i = 0; i < BLOCK_SIZE / 2; i++) {
            srand(i);
            buffer[i] = (short)rand();
        }
        RecordBlock *block = (RecordBlock*)buffer;
        uint16_t checksum = block->header.compute();
        block->header.checksum = checksum;
        Assert::AreEqual(1, block->header.check());
        free(buffer);
    }


    TEST_METHOD(BlockAddRecord) {
        {
            RecordBlock *block = (RecordBlock*)malloc(BLOCK_SIZE);
            char data[10] = { 8, 0, 'H', 'e', 'l', 'l', 'o', '\0' };
            unsigned size = 8;
            block->init();
            // add multi records
            int len = 300;
            for (int i = 0; i < len; i++) {
                uint32_t pos;
                Assert::AreEqual(1, block->addRecord((Record*)data, &pos));
                Assert::AreEqual((uint32_t)i, pos);
            }
            // count
            Assert::AreEqual(len, (int)block->count);

            // free
            unsigned freeP = BLOCK_SIZE - (size)* len;
            unsigned actu = block->free;
            Assert::AreEqual(freeP, actu);
            free(block);
        }
        {
            RecordBlock *block = (RecordBlock*)malloc(BLOCK_SIZE);
            char data[10] = { 2, 0 };
            unsigned size = 2;
            block->init();
            // add multi records
            int len = 1019;
            for (int i = 0; i < len; i++) {
                Assert::AreEqual(1, block->addRecord((Record*)data, NULL));
            }
            Assert::AreEqual(0, block->addRecord((Record*)data, NULL));
            // count
            Assert::AreEqual(len, (int)block->count);

            // free
            unsigned freeP = BLOCK_SIZE - (size)* len;
            unsigned actu = block->free;
            Assert::AreEqual(freeP, actu);
            free(block);
        }
    }

    TEST_METHOD(BLOCKGetRecord) {
        RecordBlock *block = (RecordBlock*)malloc(BLOCK_SIZE);
        char data[10] = { 8, 0, 'H', 'e', 'l', 'l', 'o', '\0' };
        unsigned size = 8;
        block->init();
        // add record
        int len = 300;
        for (int i = 0; i < len; i++) {
            Assert::AreEqual(1, block->addRecord((Record*)data, NULL));
        }
        for (int i = 0; i < len; i++) {
            // check
            Record * rec = (Record*)block->getRecord(i);
            Assert::AreEqual(size, (unsigned)(rec->size));
            Assert::AreEqual(std::string(data + 2), std::string((char*)rec->data));
        }
        free(block);
    }

    TEST_METHOD(BLOCKDelRecord) {
        RecordBlock *block = (RecordBlock*)malloc(BLOCK_SIZE);
        char data[15] = { 13, 0, 'H', 'e', 'l', 'l', 'o','w','o','r' ,'l','d','\0' };
        unsigned size = 13;
        block->init();

        int len = 200;
        for (int i = 0; i < len; i++) {
            Assert::AreEqual(1, block->addRecord((Record*)data, NULL));
        }
        for (int i = 0; i < len; i++) {
            Assert::AreEqual(1, block->delRecord(i));
        }
        Assert::AreEqual(len, (int)block->count);
        for (int i = 0; i < len; i++) {
            Assert::AreEqual(0, (int)block->directory[i]);
        }
        free(block);
    }

    TEST_METHOD(BLOCKUpdateRecord) {
        RecordBlock *block = (RecordBlock*)malloc(BLOCK_SIZE);
        char data[10] = { 8, 0, 'H', 'e', 'l', 'l', 'o', '\0' };
        unsigned size = 8;
        block->init();

        int len = 100;
        for (int i = 0; i < len; i++) {
            Assert::AreEqual(1, block->addRecord((Record*)data, NULL));
        }

        {
            char newData[] = { 8, 0, 'h', 'i','h','o','e', '\0' };
            for (int i = 0; i < len; i++) {
                Assert::AreEqual(1, block->updateRecord(i, (Record*)newData));
            }

            for (int i = 0; i < len; i++) {
                Record * rec = (Record*)block->getRecord(i);
                Assert::AreEqual(std::string(newData + 2), std::string((char*)rec->data));
            }
        }
        {
            char newData[] = { 13, 0, 'h', 'e', 'l', 'l','o','w','o','r','l','d', '\0' };
            for (int i = 0; i < len; i++) {
                Assert::AreEqual(1, block->updateRecord(i, (Record*)newData));
            }

            for (int i = 0; i < len; i++) {
                Record * rec = (Record*)block->getRecord(i);
                Assert::AreEqual(std::string(newData + 2), std::string((char*)rec->data));
            }
        }
        free(block);
    }

    };
}
