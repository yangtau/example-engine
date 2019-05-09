#include "CppUnitTest.h"
#include "stdafx.h"
#include <string>
#include "../Project/buffer.h"


using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest {
    TEST_CLASS(BufferManagerUnitTest) {
public:
    TEST_METHOD(BufferManagerTest) {
        // test free list
        BufferManager manager = BufferManager();
        for (int i = 0; i < 20; i++) {
            void *t = manager.allocBlock();
            Assert::AreEqual(0ul, (unsigned long)t % BLOCK_SIZE);
        }
    }
    };
}
