#pragma once
#include <iostream>
#include <string>
#include "btree.h"

#define assert_equal(a, b, msg)                                  \
  do {                                                           \
    if (!(a == b)) {                                             \
      fprintf(stderr, "#func: %s#line:%d ", __func__, __LINE__); \
      fprintf(stderr, "%s\n", msg);                              \
      getchar();                                                 \
      exit(-1);                                                  \
    }                                                            \
  } while (0)

using namespace std;

string s = "HelloWorld";

int cmp(const void *a, const void *b, void *) { return *(int *)a - *(int *)b; }

void insertIntoBTree(BTree *btree, int a, int b) {
  for (int i = a; i < b; i++) {
    string value = to_string(i) + s + to_string(i * 2);
    int res = btree->put(&i, value.c_str(), value.length() + 1);
    assert_equal(1, res, to_string(i).c_str());
  }
}

void testPutAndQuery() {
  {
    // put
    BTree b(sizeof(int), cmp);
    b.create("test.db");
    assert_equal(1, b.open("test.db"), "");
    insertIntoBTree(&b, 400, 500);
    insertIntoBTree(&b, 0, 200);
    insertIntoBTree(&b, 250, 300);
  }

  {
    // put duplicate key
    BTree b(sizeof(int), cmp);
    assert_equal(1, b.open("test.db"), "");
    int key = 401;
    assert_equal(0, b.put(&key, NULL, 0), "");
    key = 20;
    assert_equal(0, b.put(&key, NULL, 0), "");
    key = 260;
    assert_equal(0, b.put(&key, NULL, 0), "");
  }
  {
    // [400, 500)
    // [0, 200)
    // [250, 300)
    BTree bt(sizeof(int), cmp);
    assert_equal(1, bt.open("test.db"), "");
    BTree::Iterator it = bt.iterator();

    for (int i = 0; i < 200; i++) {
      assert_equal(1, it.locate(&i, 0), "");
      string eval = to_string(i) + s + to_string(i * 2);
      string rval = string((char *)it.getValue());
      assert_equal(eval, rval, (eval + "!=" + rval).c_str());
    }

    //[400, 500)
    int a = 400;
    assert_equal(1, it.locate(&a, 0), "");
    do {
      string eval = to_string(a) + s + to_string(a * 2);
      a++;
      string rval = string((char *)it.getValue());
      assert_equal(eval, rval, (eval + "!=" + rval).c_str());
    } while (it.next());
    assert_equal(a, 500, "");

    // [250, 300)
    int b = 300;
    assert_equal(1, it.locate(&b, 2), "");
    b--;
    do {
      string eval = to_string(b) + s + to_string(b * 2);
      b--;
      if (b < 250) {
        break;
      }
      string rval = string((char *)it.getValue());
      assert_equal(eval, rval, (eval + "!=" + rval).c_str());
    } while (it.prev());
    assert_equal(249, b, "");
    assert_equal(1, it.prev(), "");
    assert_equal(199, *(int *)it.getKey(), "");
  }
}

// testPutAndQuery must run before this test.
void testIterator() {
  // [400, 500)
  // [0, 200)
  // [250, 300)
  BTree btree(sizeof(int), cmp);
  assert_equal(1, btree.open("test.db"), "");
  BTree::Iterator it = btree.iterator();

  {
    // first & last
    assert_equal(1, it.first(), "");
    assert_equal(0, *(int *)it.getKey(), "");

    assert_equal(1, it.last(), "");
    assert_equal(499, *(int *)it.getKey(), "");
  }

  {
    // key or next
    int key = -1;
    assert_equal(1, it.locate(&key, 1), "");
    assert_equal(key + 1, *(int *)it.getKey(), "");
    assert_equal(0, it.prev(), "");
    assert_equal(1, it.next(), "");

    key = 200;
    assert_equal(1, it.locate(&key, 1), "");
    assert_equal(250, *(int *)it.getKey(), "");
    assert_equal(1, it.prev(), "");
    assert_equal(1, it.next(), "");

    key = 500;
    assert_equal(0, it.locate(&key, 1), "");
  }
  {
    // key or prev
    // [400, 500)
    // [0, 200)
    // [250, 300)
    int key = 400;
    assert_equal(1, it.locate(&key, 2), "");
    assert_equal(400, *(int *)it.getKey(), "");
    assert_equal(1, it.prev(), "");
    assert_equal(299, *(int *)it.getKey(), "");

    key = 500;
    assert_equal(1, it.locate(&key, 2), "");
    assert_equal(499, *(int *)it.getKey(), "");
    // next
    assert_equal(0, it.next(), "");

    key = 250;
    assert_equal(1, it.locate(&key, 2), "");
    assert_equal(250, *(int *)it.getKey(), "");
    // prev
    assert_equal(1, it.prev(), "");
    assert_equal(199, *(int *)it.getKey(), "");

    key = 600;
    assert_equal(1, it.locate(&key, 2), "");
    assert_equal(499, *(int *)it.getKey(), "");

    key = 0;
    assert_equal(1, it.locate(&key, 2), "");
    assert_equal(0, *(int *)it.getKey(), "");

    key = -1;
    assert_equal(0, it.locate(&key, 2), "");
  }
  {
    // afer key
    // [400, 500)
    // [0, 200)
    // [250, 300)
    int key = 0;
    assert_equal(1, it.locate(&key, 3), "");
    assert_equal(1, *(int *)it.getKey(), "");

    key = 199;
    assert_equal(1, it.locate(&key, 3), "");
    assert_equal(250, *(int *)it.getKey(), "");

    key = 499;
    assert_equal(0, it.locate(&key, 3), "");

    key = -200;
    assert_equal(1, it.locate(&key, 3), "");
    assert_equal(0, *(int *)it.getKey(), "");
  }
  {
    // before key
    // [400, 500)
    // [0, 200)
    // [250, 300)
    int key = -200;
    assert_equal(0, it.locate(&key, 4), "");

    key = 500;
    assert_equal(1, it.locate(&key, 4), "");
    assert_equal(499, *(int *)it.getKey(), "");

    key = 400;
    assert_equal(1, it.locate(&key, 4), "");
    assert_equal(299, *(int *)it.getKey(), "");

    key = 225;
    assert_equal(1, it.locate(&key, 4), "");
    assert_equal(199, *(int *)it.getKey(), "");
  }
}

void testRemove() {
  {  // [400, 500)
    // [0, 200)
    // [250, 300)
    BTree bt(sizeof(int), cmp);
    assert_equal(1, bt.open("test.db"), "");
    BTree::Iterator it = bt.iterator();
    assert_equal(1, it.first(), "");

    do {
      int key = *(int *)it.getKey();
      if (key % 2 == 1) it.remove();
    } while (it.next());
  }
  {
    // [0, 200)
    // [250, 300)
    // [400, 500)
    BTree bt(sizeof(int), cmp);
    assert_equal(1, bt.open("test.db"), "");
    BTree::Iterator it = bt.iterator();

    for (int i = 0; i < 200; i++) {
      if (i % 2 == 1) {
        assert_equal(0, it.locate(&i, 0), "");
        continue;
      }
      assert_equal(1, it.locate(&i, 0), "");
      string eval = to_string(i) + s + to_string(i * 2);
      string rval = string((char *)it.getValue());
      assert_equal(eval, rval, (eval + "!=" + rval).c_str());
    }

    //[400, 500)
    int a = 400;
    assert_equal(1, it.locate(&a, 0), "");
    do {
      string eval = to_string(a) + s + to_string(a * 2);
      a += 2;
      string rval = string((char *)it.getValue());
      assert_equal(eval, rval, (eval + "!=" + rval).c_str());
    } while (it.next());
    assert_equal(500, a, "");

    // [250, 300)
    int b = 300;
    assert_equal(1, it.locate(&b, 2), "");
    b -= 2;
    do {
      string eval = to_string(b) + s + to_string(b * 2);
      b -= 2;
      if (b < 250) {
        break;
      }
      string rval = string((char *)it.getValue());
      assert_equal(eval, rval, (eval + "!=" + rval).c_str());
    } while (it.prev());
    assert_equal(248, b, "");
    assert_equal(1, it.prev(), "");
    assert_equal(198, *(int *)it.getKey(), "");
  }

  {
    // [0, 200)
    // [250, 300)
    // [400, 500)
    // even
    BTree bt(sizeof(int), cmp);
    assert_equal(1, bt.open("test.db"), "");
    BTree::Iterator it = bt.iterator();
    assert_equal(1, it.first(), "");
    assert_equal(0, *(int *)it.getKey(), "");
    assert_equal(1, it.last(), "");
    assert_equal(498, *(int *)it.getKey(), "");

    int key;
    // key or next
    key = 2;
    assert_equal(1, it.locate(&key, 1), "");
    assert_equal(2, *(int *)it.getKey(), "");
    key = 251;
    assert_equal(1, it.locate(&key, 1), "");
    assert_equal(252, *(int *)it.getKey(), "");

    // key or prev
    key = 400;
    assert_equal(1, it.locate(&key, 2), "");
    assert_equal(400, *(int *)it.getKey(), "");
    key = 499;
    assert_equal(1, it.locate(&key, 2), "");
    assert_equal(498, *(int *)it.getKey(), "");

    // after key
    key = 1;
    assert_equal(1, it.locate(&key, 3), "");
    assert_equal(2, *(int *)it.getKey(), "");
    key = 250;
    assert_equal(1, it.locate(&key, 3), "");
    assert_equal(252, *(int *)it.getKey(), "");

    // before key
    key = 0;
    assert_equal(0, it.locate(&key, 4), "");

    key = 250;
    assert_equal(1, it.locate(&key, 4), "");
    assert_equal(198, *(int *)it.getKey(), "");
  }
}

void testReinsert() {
  {  // [0, 200)
    // [250, 300)
    // [400, 500)
    // even
    BTree btree(sizeof(int), cmp);
    assert_equal(1, btree.open("test.db"), "");
    for (int i = 0; i < 200; i++) {
      if (i % 2 == 0) continue;
      string value = to_string(i) + s + to_string(i * 2);
      int res = btree.put(&i, value.c_str(), value.length() + 1);
      assert_equal(1, res, to_string(i).c_str());
    }
  }
  {
    BTree bt(sizeof(int), cmp);
    assert_equal(1, bt.open("test.db"), "");
    BTree::Iterator it = bt.iterator();

    for (int i = 0; i < 200; i++) {
      assert_equal(1, it.locate(&i, 0), "");
      string eval = to_string(i) + s + to_string(i * 2);
      string rval = string((char *)it.getValue());
      assert_equal(eval, rval, (eval + "!=" + rval).c_str());
    }
  }
}

void testConstValue() {
  {
    BTree btree(sizeof(int), cmp, NULL, sizeof(int));
    assert_equal(1, btree.create("test.db"), "");
    assert_equal(1, btree.open("test.db"), "");

    for (int i = 0; i < 400; i += 2) {
      int value = i * i + i;
      assert_equal(1, btree.put(&i, &value), "");
    }
  }
  {
    BTree btree(sizeof(int), cmp, NULL, sizeof(int));
    assert_equal(1, btree.open("test.db"), "");
    BTree::Iterator it = btree.iterator();
    int key = 0;
    assert_equal(1, it.locate(&key, 0), "");
    do {
      assert_equal(key * key + key, *(int *)it.getValue(), "");
      key += 2;
    } while (it.next());
    assert_equal(400, key, "");
  }
}

void test() {
  testPutAndQuery();
  testIterator();
  testRemove();
  testReinsert();
  testConstValue();
}