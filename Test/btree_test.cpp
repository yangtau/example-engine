#include "CppUnitTest.h"
#include "stdafx.h"
#include <string>
#include "../Project/btree.h"
#include "../Project/storage.h"


using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest {
    TEST_CLASS(BPlusTreeUnitTest) {
public:
    TEST_METHOD(Test) {
        StorageManager s = StorageManager("btree-test.db");
        BTree btree = BTree(s);
        KeyValue kv;
        kv.key = 20;
        kv.value = 300;
        btree.insert(kv);

        kv.key = 10;
        btree.insert(kv);

        kv.key = 30;
        btree.insert(kv);

        kv.key = 15;
        btree.insert(kv);

        kv.key = 80;
        btree.insert(kv);

        kv.key = 70;
        btree.insert(kv);
    }
    };
}
