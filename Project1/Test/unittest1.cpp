#include "CppUnitTest.h"
#include "stdafx.h"
#include "../Project1/file.h"
#include "../Project1/block.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest {
	TEST_CLASS(File_UnitTest) {
public:
	TEST_METHOD(File_Create) {
		File file;
		int res = file.create("", 0);
		Assert::AreEqual(res, 0);
	}
	TEST_METHOD(Block_Check) {
		short *buffer = (short*) malloc(BLOCK_SIZE);
		for (int i = 0; i < BLOCK_SIZE / 2; i++) {
			srand(i);
			buffer[i] = (short)rand();
		}
		Block *block = (Block*)buffer;
		uint16_t checksum = block->header.compute();
		block->header.checksum = checksum;
		Assert::AreEqual(1, block->header.check());
	}
	};
}
