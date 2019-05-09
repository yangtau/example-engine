#include <iostream>
#include "file.h"
int main() {
    File file;
    file.create("Hello.db", 32);
    return 0;
}