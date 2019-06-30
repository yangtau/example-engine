#include <iostream>
#include "test.h";
//#include "matu.h"
using std::count;

int main(int argvc, char **args) {
    //switch (args[1][0]) {
    //  case '1':
    //    testM1();
    //      break;
    //  case '2':
    //    testM2();
    //      break;
    //  case '3':
    //    testM3();
    //      break;
    //  default:
    //    break;
    //}
    test();
    cout << "done\n";
    getchar();
    return 0;
}
