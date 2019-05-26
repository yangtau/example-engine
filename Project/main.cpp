#include <iostream>
#include "matu.h"
#include <ctime>
int main() {
    auto start = clock();
    test1();
    auto end = clock();
    std::cout << "Time: " << (end - start)*1.0 / CLOCKS_PER_SEC << std::endl;
    getchar();

    return 0;
}