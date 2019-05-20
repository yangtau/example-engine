#include "CppUnitTest.h"
#include "stdafx.h"
#include <string>
#include "../Project/btree.h"
#include "../Project/storage.h"
#define DEBUG_BTREE


using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest {
    TEST_CLASS(BPlusTreeUnitTest) {
public:
    TEST_METHOD(insert) {
        StorageManager s = StorageManager("btree-test.db");
        BTree btree = BTree(s);
        KeyValue kvs[12] = {
            {20, 200}, {10, 200}, {15, 200},
            {30, 900}, {80, 300}, {60, 200},
            {70, 110}, {0, 900}, {5, 700},
            {22, 300}, {45, 300}, {25, 100}
        };

        for (auto &i : kvs) {
            Assert::AreEqual(1, btree.insert(i));
        }
        Assert::AreEqual(1, btree.insert(kvs[11]));
    }

    TEST_METHOD(remove) {
        StorageManager s = StorageManager("btree-test.db");
        BTree btree = BTree(s);

        KeyValue kvs[12] = {
            {20, 200}, {10, 200}, {15, 200},
            {30, 900}, {80, 300}, {60, 200},
            {70, 110}, {0, 900}, {5, 700},
            {22, 300}, {45, 300}, {25, 100}
        };

        for (auto &i : kvs) {
            Assert::AreEqual(1, btree.insert(i));
        }
        for (auto &i : kvs) {
            Assert::AreEqual(1, btree.remove(i.key));
        }
    }

    TEST_METHOD(search) {
        StorageManager s = StorageManager("btree-test.db");
        BTree btree = BTree(s);

        KeyValue kvs[12] = {
            {20, 200}, {10, 200}, {15, 200},
            {30, 900}, {80, 300}, {60, 200},
            {70, 110}, {0, 900}, {5, 700},
            {22, 300}, {45, 300}, {25, 100}
        };

        for (auto &i : kvs) {
            Assert::AreEqual(1, btree.insert(i));
        }
        for (int i = 0; i < 12; i++) {
            Assert::AreEqual(kvs[i].value, btree.search(kvs[i].key).value);
        }
        Assert::AreEqual(0u, btree.search(44).value);
        Assert::AreEqual(0u, btree.search(33).value);
        Assert::AreEqual(0u, btree.search(55).value);
        Assert::AreEqual(0u, btree.search(11).value);
    }


    };
}
