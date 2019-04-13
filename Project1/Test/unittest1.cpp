#include "CppUnitTest.h"
#include "stdafx.h"
#include "../Project1/file.h"
#include "../Project1/block.h"
#include <string>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest {
	TEST_CLASS(File_UnitTest) {
public:


	TEST_METHOD(Block_Check) {
		short *buffer = (short*)malloc(BLOCK_SIZE);
		for (int i = 0; i < BLOCK_SIZE / 2; i++) {
			srand(i);
			buffer[i] = (short)rand();
		}
		Block *block = (Block*)buffer;
		uint16_t checksum = block->header.compute();
		block->header.checksum = checksum;
		Assert::AreEqual(1, block->header.check());
		free(buffer);
	}


	TEST_METHOD(Block_AddRecord) {
		Block *block = (Block*)malloc(BLOCK_SIZE);
		char data[10] = { 6, 0, 0, 0, 'H', 'e', 'l', 'l', 'o', '\0' };
		unsigned size = 6;
		block->header.count = 0;
		block->header.free = sizeof(BlockHeader);
		// add multi records
		int len = 300;
		for (int i = 0; i < len; i++) {
			Assert::AreEqual(1, block->addRecord((Record*)data));
		}
		// count
		Assert::AreEqual(len, (int)block->header.count);
		// offset
		unsigned exceptOffset = sizeof(BlockHeader) + (size + sizeof(RecordHeader)) * (len - 1);
		unsigned offset = block->getTailer()->slots[0];
		Assert::AreEqual(exceptOffset, offset);
		// free
		unsigned freeP = sizeof(BlockHeader) + (size + sizeof(RecordHeader)) * len;
		unsigned actu = block->header.free;
		Assert::AreEqual(freeP, actu);
		free(block);
	}

	TEST_METHOD(BLOCK_GetRecord) {
		Block *block = (Block*)malloc(BLOCK_SIZE);
		char data[10] = { 6, 0, 0, 0, 'H', 'e', 'l', 'l', 'o', '\0' };
		unsigned size = 6;
		block->header.count = 0;
		block->header.free = sizeof(BlockHeader);
		// add record
		block->header.count = 0;
		block->header.free = sizeof(BlockHeader);
		int len = 300;
		for (int i = 0; i < len; i++) {
			Assert::AreEqual(1, block->addRecord((Record*)data));
		}
		for (int i = 0; i < len; i++) {
			// check
			Record * rec = (Record*)block->getRecord(i);
			Assert::AreEqual(size, (unsigned)(rec->header.size));
			Assert::AreEqual(std::string(data + 4), std::string((char*)rec->getData()));
		}
		free(block);
	}

	TEST_METHOD(BLOCK_DelRecord) {
		Block *block = (Block*)malloc(BLOCK_SIZE);
		char data[10] = { 6, 0, 0, 0, 'H', 'e', 'l', 'l', 'o', '\0' };
		unsigned size = 6;
		block->header.count = 0;
		block->header.free = sizeof(BlockHeader);
		// add record
		block->header.count = 0;
		block->header.free = sizeof(BlockHeader);
		int len = 300;
		for (int i = 0; i < len; i++) {
			Assert::AreEqual(1, block->addRecord((Record*)data));
		}
		for (int i = 0; i < len; i++) {
			Assert::AreEqual(1, block->delRecord(i));
		}
		Assert::AreEqual(len, (int)block->header.count);
		for (int i = 0; i < len; i++) {
			Assert::AreEqual(0, (int)block->getTailer()->slots[i]);
		}
		free(block);
	}

	TEST_METHOD(BLOCK_UpdateRecord) {
		Block *block = (Block*)malloc(BLOCK_SIZE);
		char data[10] = { 6, 0, 0, 0, 'H', 'e', 'l', 'l', 'o', '\0' };
		unsigned size = 6;
		block->header.count = 0;
		block->header.free = sizeof(BlockHeader);
		int len = 300;
		for (int i = 0; i < len; i++) {
			Assert::AreEqual(1, block->addRecord((Record*)data));
		}
		char newData[10] = { 6, 0, 0, 0, 'W', 'o', 'r', 'l', 'd', '\0' };
		for (int i = 0; i < len; i++) {
			Assert::AreEqual(1, block->updateRecord(i, (Record*)newData));
		}
		for (int i = 0; i < len; i++) {
			// check
			Record * rec = (Record*)block->getRecord(i);
			Assert::AreEqual(size, (unsigned)(rec->header.size));
			Assert::AreEqual(std::string(newData + 4), std::string((char*)rec->getData()));
		}
		free(block);
	}


	TEST_METHOD(File_Create) {
		File file;
		int res = file.create("Hello", 20);
		Assert::AreEqual(res, 1);
	}

	TEST_METHOD(File_AllocateBlock) {
		File file;
		int res = file.create("Hello", 20);
		Assert::AreEqual(res, 1);
		Block *block = file.allocateBlock(3);
		int re = block != NULL;
		Assert::AreEqual(1, re);
		Assert::AreEqual(0, (int)block->header.count);
		Assert::AreEqual((int)sizeof(BlockHeader), (int)block->header.free);
	}

	TEST_METHOD(File_WriteBlock) {
		File file;
		int res = file.create("Hello", 20);
		Assert::AreEqual(res, 1);
		Block *block = file.allocateBlock(3);

		char data[10] = { 6, 0, 0, 0, 'H', 'e', 'l', 'l', 'o', '\0' };
		int re = block->addRecord((Record*)data);
		Assert::AreEqual(re, 1);
		block->header.next = 0;
		file.writeBlock(3, block);
		block = file.allocateBlock(1);
		block = file.allocateBlock(3);
		Record * rec = (Record*)block->getRecord(0);
		Assert::AreEqual(std::string(data + 4), std::string((char*)rec->getData()));
	}

	};
}
