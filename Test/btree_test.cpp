#include "CppUnitTest.h"
#include "stdafx.h"
#include <string>
#include "../Project/btree.h"
#include "../Project/storage.h"
#define BTREE_DEBUG


using namespace Microsoft::VisualStudio::CppUnitTestFramework;
int cmp(const void*a, const void *b, void*p) {
    return *(uint8_t*)a - *(uint8_t*)b;
}
int _cmp(const void*a, const void *b) {
    return *(uint8_t*)a - *(uint8_t*)b;
}
namespace UnitTest {
    TEST_CLASS(BPlusTreeUnitTest) {
public:
    uint8_t keylen = sizeof(uint8_t), vallen = sizeof(uint8_t);



    TEST_METHOD(NodeBlockTest) {
        NodeBlock* node = (NodeBlock*)malloc(BLOCK_SIZE);
        node->init(BLOCK_TYPE_DATA, keylen, vallen);
        node->setCmp(cmp, NULL);

        // empty
        Assert::AreEqual(true, node->empty());
        uint8_t keys[30] = {
            23,47,93,66,100,84,44,92,11,96,
            70,37,32,75,91,17,56,66,90,68,
            58,79,32,93,53,21,54,14,15,45 };
        uint8_t values[30] = {
            32,22,76,79,65,90,5,11,21,29,
            31,39,17,68,71,99,35,38,85,6,
            56,50,73,9,79,10,20,46,63,47 };

        // insert
        for (int i = 0; i < 30; i++) {
            node->insert(keys + i, values + i, node->lub(keys + i));
        }

        // maxKey
        uint8_t maxK = *(uint8_t*)node->maxKey();
        Assert::AreEqual((uint8_t)100, maxK);

        qsort(keys, 30, sizeof(uint8_t), _cmp);

        // find & getKey
        Assert::AreEqual(0, node->find(keys + 0));
        Assert::AreEqual(29, node->find(keys + 29));
        for (int i = 0; i < 30; i++) {
            Assert::AreEqual(keys[i], *(uint8_t*)node->getKey(i));
        }


        // remove
        node->remove(0);
        Assert::AreEqual(-1, node->find(keys + 0));

        // set, 
        uint8_t newval = 33;
        uint8_t newkey = 200;
        node->set(20, &newkey, &newval);
        Assert::AreEqual(newval, *(uint8_t*)node->getValue(20));
        Assert::AreEqual(newkey, *(uint8_t*)node->getKey(20));

        node->removeByFlag(20);
        Assert::AreEqual(true, node->removed(20));

        // split
        uint16_t cnt = node->count;
        NodeBlock* next = (NodeBlock*)malloc(BLOCK_SIZE);
        next->init(BLOCK_TYPE_DATA, keylen, vallen);
        next->setCmp(cmp, NULL);
        node->split(next);

        Assert::AreEqual(((cnt + 1) / 2), (int)node->count);
        Assert::AreEqual(((cnt) / 2), (int)next->count);

        int index = next->find(keys + 29);
        Assert::AreEqual(true, index == next->count - 1);
    }

    TEST_METHOD(BTreeTest) {
        StorageManager s;
        s.create("tree.db");
        s.open("tree.db");
        const int len = 27;
        uint8_t keys[len] = {
            23,47,93,84,100,44,92,11,96,
            70,37,75,91,17,56,66,90,68,
            58,79,32,53,21,54,14,15,45 };
        uint8_t values[len] = {
            32,22,76,79,65,90,5,11,21,29,
            31,39,17,68,71,99,35,38,85,6,
            56,50,73,9,10,20,46 };
        {
            BTree b(keylen, vallen, s, cmp);
            for (int i = 0; i < len; i++) {
                Assert::AreEqual(1, b.put(keys + i, values + i));
            }
        }
        {
            // get
            BTree b(keylen, vallen, s, cmp);
            uint8_t v;
            uint8_t key = 200;
            Assert::AreEqual(0, b.get(&key, &v));
            key = 50;
            Assert::AreEqual(0, b.get(&key, &v));
            key = 99;
            Assert::AreEqual(0, b.get(&key, &v));
            key = 67;
            Assert::AreEqual(0, b.get(&key, &v));
            key = 59;
            Assert::AreEqual(0, b.get(&key, &v));
            key = 1;
            Assert::AreEqual(0, b.get(&key, &v));

            for (int i = 0; i < len; i++) {
                uint8_t v;
                int res = b.get(keys + i, &v);
                Assert::AreEqual(1, res);
                Assert::AreEqual(values[i], v);
            }
        }
        {
            // remove
            BTree b(keylen, vallen, s, cmp);
            for (int i = 0; i < len; i++) {
                Assert::AreEqual(1, b.remove(keys + i));
            }
            //check remove
            for (int i = 0; i < len; i++) {
                Assert::AreEqual(0, b.get(keys + i, NULL));
            }

            // insert again
            for (int i = 0; i < len; i++) {
                Assert::AreEqual(1, b.put(keys + i, values + len - i - 1));
            }
            //check
            for (int i = 0; i < len; i++) {
                uint8_t val = -1;
                Assert::AreEqual(1, b.get(keys + i, &val));
                Assert::AreEqual(values[len - 1 - i], val);
            }

            // remove agin
            for (int i = 0; i < len; i++) {
                Assert::AreEqual(1, b.remove(keys + i));
            }
            // remove again and agin
            for (int i = 0; i < len; i++) {
                Assert::AreEqual(0, b.remove(keys + i));
            }

        }
        s.close();
    }

    TEST_METHOD(BTreeIteratorTest) {
        StorageManager s;
        s.create("tree.db");
        s.open("tree.db");
        const int len = 27;
        uint8_t keys[len] = {
            23,47,93,84,100,44,92,11,96,
            70,37,75,91,17,56,66,90,68,
            58,79,32,53,21,54,14,15,45 };
        uint8_t values[len] = {
            32,22,76,79,65,90,5,11,21,29,
            31,39,17,68,71,99,35,38,85,6,
            56,50,73,9,10,20,46 };
        BTree b(keylen, vallen, s, cmp);

        BTree::Iterator *it = b.iterator();

        {// open
            uint8_t lo = 0, hi = 20;
            Assert::AreEqual(0, it->open(&lo, &hi));
        }

        {   // insert
            for (int i = 0; i < len; i++) {
                Assert::AreEqual(1, b.put(keys + i, values + i));
            }
        }
        // sorted keys, values
        uint8_t k[] = {
            11, 14, 15, 17, 21,//0~4
            23, 32, 37, 44,45, 47,
            53,//5~11
            54, 56, 58, 66, 68, 70,
            75, 79, 84, 90, 91, 92, 93, 96, 100 };
        uint8_t v[] = { 11, 10, 20, 68, 73,
            32, 56, 31, 90, 46,
            22, 50, 9, 71, 85, 99,
            38, 29, 39, 6, 79, 35, 17, 5, 76, 21, 65 };
        {
            Assert::AreEqual(1, it->open(NULL, NULL));
            int  i = 0;
            uint8_t key, value;
            do {
                key = -1, value = -1;
                it->get(&key, &value);
                Assert::AreEqual(k[i], key);
                Assert::AreEqual(v[i], value);
                i++;
            } while (it->next() == 1);

            Assert::AreEqual(len, i);

            it->close();
        }
        {   // iterator
            uint8_t lo = 11, hi = 101;
            Assert::AreEqual(1, it->open(&lo, &hi));
            int  i = 0;
            uint8_t key, value;
            do {
                key = -1, value = -1;
                it->get(&key, &value);
                Assert::AreEqual(k[i], key);
                Assert::AreEqual(v[i], value);
                i++;
            } while (it->next() == 1);

            Assert::AreEqual(len, i);

            it->close();
        }

        {
            int lo = 10, hi = 20;

            // open with null lo
            Assert::AreEqual(1, it->open(NULL, k + hi));

            int  i = 0;
            uint8_t key, value;
            do {
                key = -1, value = -1;
                it->get(&key, &value);
                Assert::AreEqual(k[i], key);
                Assert::AreEqual(v[i], value);
                i++;
            } while (it->next() == 1);
            Assert::AreEqual(hi, i);

            // open with the iteraor opend
            Assert::AreEqual(0, it->open(k + lo, NULL));
            it->close();

            Assert::AreEqual(1, it->open(k + lo, NULL));
            i = lo;
            do {
                key = -1, value = -1;
                it->get(&key, &value);
                Assert::AreEqual(k[i], key);
                Assert::AreEqual(v[i], value);
                i++;
            } while (it->next() == 1);
            Assert::AreEqual(len, i);
            it->close();
        }

        {
            //set
            // 5~11
            uint8_t hi = 50, lo = 22;
            Assert::AreEqual(1, it->open(&lo, &hi));
            int i = 5;
            do {
                uint8_t key = -1, value = -1;
                it->get(&key, &value);
                Assert::AreEqual(k[i], key);
                Assert::AreEqual(v[i], value);
                it->set(v + i + 5);
                i++;
            } while (it->next() == 1);
            Assert::AreEqual(11, i);

            it->close();

            // check
            Assert::AreEqual(1, it->open(&lo, &hi));
            i = 5;
            do {
                uint8_t key = -1, value = -1;
                it->get(&key, &value);
                Assert::AreEqual(k[i], key);
                Assert::AreEqual(v[i + 5], value);
                i++;
            } while (it->next() == 1);
            Assert::AreEqual(11, i);

            it->close();
        }

        {
            // remove 
            // 5~11
            // 11, 14, 15, 17, 21,//0~4
            //23, 32, 37, 44,45, 47, 
            //53,//5~11
            // 54, 56, 58, 66, 68, 70,
            // remove 23 32 37 44 45 47 
            uint8_t hi = 50, lo = 22;
            Assert::AreEqual(1, it->open(&lo, &hi));
            int i = 5;
            do {
                uint8_t key = -1, value = -1;
                it->get(&key, &value);
                Assert::AreEqual(k[i], key);
                it->remove();
                i++;
            } while (it->next() == 1);
            Assert::AreEqual(11, i);

            it->close();

            //check remove
            Assert::AreEqual(0, it->open(&lo, &hi));
            

            hi = k[13];
            Assert::AreEqual(1, it->open(&lo, &hi));
            do {
                uint8_t key = -1, value = -1;
                it->get(&key, &value);
                Assert::AreEqual(k[i], key);
                Assert::AreEqual(v[i], value);
                it->remove();
                i++;
            } while (it->next() == 1);
            Assert::AreEqual(13, i);
        }

        delete it;
    }

    };
};
