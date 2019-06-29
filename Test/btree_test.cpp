
#define BTREE_DEBUG
#include <string>
#include "CppUnitTest.h"
#include "stdafx.h"
#include "../Project/btree.h"
using std::to_string;
using std::string;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

int cmp(const void *a, const void *b, void *p) { return *(u8 *)a - *(u8 *)b; }
int _cmp(const void *a, const void *b) { return *(u8 *)a - *(u8 *)b; }
string s = "Hello_World";

namespace UnitTest {
    TEST_CLASS(BPlusTreeUnitTest) {
public:

    // [400, 500)
    // [0, 200)
    // [250, 300)

  

    };
};
