#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <ctime>
#include "matu.h"
#include <vector>
#include <string>
#include <windows.h>
#include <fstream>
#include <iostream>
#include <ctime>
using namespace std;

void initial();
void insert(std::vector<RowData> rows);//插入数据默认按照主键升序
std::vector<RowData> query(string sql); //查询数据默认按照主键升序
void del(string sql);
void update(string sql);
int cmd = 0;

bool CheckFile(string fileName) {
    ifstream file(fileName, ios::binary | ios::in);
    if (!file.is_open())
        return false;
    file.seekg(0, ios::beg);
    char result[4] = { 0xc1,0xc6,0xf0,0x1e };
    while (file.good()) {
        char buff[4];
        if (file.read(buff, sizeof(buff))) {
            if (strncmp(buff, result, sizeof(buff)) != 0) {
                return false;
            }
        }
        file.seekg(4092, ios::cur);
    }
    file.close();
    return true;
}

void testM1() {
    initial();

    for (int i = 0; i < 10000; i++) {
        vector<RowData> rowDatas;
        RowData rowData;
        rowData.id = i;
        rowData.name = string("ccg").append(to_string(i));
        rowData.number = string("20190603").append(to_string(i));
        rowData.sex = true;
        rowData.email = string("20190603@").append(to_string(i)).append(".com");
        rowDatas.push_back(rowData);
        insert(rowDatas);
    }

    if (!CheckFile("table.db")) {
        printf("wrong\n");
        return;
    }
    vector<RowData> result = query("select * from table where id > 500 and id <999");
    bool flag = true;
    for (int i = 501; i < 999; i++) {
        RowData rowData = result.front();
        result.erase(result.begin());
        if (rowData.id != i) {
            flag = false;
            break;
        }
        if (rowData.name.compare(string("ccg").append(to_string(i))) != 0) {
            flag = false;
            break;
        }
        if (rowData.number.compare(string("20190603").append(to_string(i))) != 0) {
            flag = false;
            break;
        }
        if (!rowData.sex) {
            flag = false;
            break;
        }
        if (rowData.email.compare(string("20190603@").append(to_string(i)).append(".com")) != 0) {
            flag = false;
            break;
        }
    }
    //del("delete from table where id > 2956 and id < 7854");
    if (flag == false) {
        printf("wrong\n");
    }
    else
        printf("yes\n");
}

void testM3() {
    update("update table set number = 20190603x where id<50 or id>9950");
    vector<RowData> rowDatas = query("select * from table where id<50 or id>9950");
    if (rowDatas.size() != 99) {
        printf("wrong\n");
        return;
    }
    for (vector<RowData>::iterator it = rowDatas.begin(); it != rowDatas.end(); it++) {
        if ((*it).number.compare(string("20190603x")) != 0) {
            printf("wrong\n");
            return;
        }
    }
    if (!CheckFile("table.db")) {
        printf("wrong\n");
        return;
    }
    printf("yes\n");
    return;
}

void testM2()
{

    int flag = true;
    del("delete from table where id > 2956 and id < 7854");
    vector<RowData> result = query("select * from table where id > 2900 and id <5000");
    for (int i = 2901; i < 2957; i++)
    //for (int i = 2991; i < 2957; i++)
    {
        //?:从2901 开始
        RowData rowData = result.front();
        result.erase(result.begin());
        if (rowData.id != i) {
            flag = false;
            break;
        }
        if (rowData.name.compare(string("ccg").append(to_string(i))) != 0) {
            flag = false;
            break;
        }
        if (rowData.number.compare(string("20190603").append(to_string(i))) != 0) {
            flag = false;
            break;
        }
        if (!rowData.sex) {
            flag = false;
            break;
        }
        if (rowData.email.compare(string("20190603@").append(to_string(i)).append(".com")) != 0) {
            flag = false;
            break;
        }
    }
    if (flag == false) {
        printf("wrong\n");
        return;
    }
    result = query("select * from table where id > 5000 and id < 7900");
    for (int i = 7854; i < 7900; i++) 
    //for (int i = 7855; i < 7900; i++) 
    {
        // ?: 从7854开始
        RowData rowData = result.front();
        result.erase(result.begin());
        if (rowData.id != i) {
            flag = false;
            break;
        }
        if (rowData.name.compare(string("ccg").append(to_string(i))) != 0) {
            flag = false;
            break;
        }
        if (rowData.number.compare(string("20190603").append(to_string(i))) != 0) {
            flag = false;
            break;
        }
        if (!rowData.sex) {
            flag = false;
            break;
        }
        if (rowData.email.compare(string("20190603@").append(to_string(i)).append(".com")) != 0) {
            flag = false;
            break;
        }
    }
    if (flag == false) {
        printf("wrong\n");
        return;
    }

    if (!CheckFile("table.db")) {
        printf("wrong\n");
        return;
    }
    printf("yes\n");
}

int main() {
    auto start = clock();
    testM1();
    //testM2();
    //testM3();
    auto end = clock();
    std::cout << "Time: " << (end - start)*1.0 / CLOCKS_PER_SEC << std::endl;
    //system("pause");
    return 0;
}
