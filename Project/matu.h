#include <iostream>
#include <string>
#include <vector>
#include "block.h"
#include "btree.h"
#include "buffer.h"
#include "storage.h"
using std::string;
using std::vector;

struct RowData {
    int id;
    bool sex;
    string name;
    string number;
    string email;
};

Record* encode(const RowData& data) {
    uint16_t nameLen = data.name.size();
    uint16_t numLen = data.number.size();
    uint16_t emailLen = data.email.size();
    Record* r = (Record*)malloc(sizeof(RecordHeader) + 14 + nameLen + emailLen +
        numLen);
    r->header.size = 14 + nameLen + emailLen + numLen;
    uint8_t* p = (uint8_t*)r + sizeof(RecordHeader);
    *(uint16_t*)p = nameLen;
    p += 2;
    *(uint16_t*)p = numLen;
    p += 2;
    *(uint16_t*)p = emailLen;

    p += 2;
    *(int*)p = data.id;
    p += 4;
    *p = data.sex;
    p += 1;

    memcpy(p, data.name.c_str(), nameLen);
    p += nameLen;
    *p = '\0';
    p++;

    memcpy(p, data.number.c_str(), numLen);
    p += numLen;
    *p = '\0';
    p++;

    memcpy(p, data.email.c_str(), emailLen);
    p += emailLen;
    *p = '\0';
    return r;
}

RowData decode(const Record* record) {
    RowData data;
    const uint8_t* p = record->data;
    uint16_t nameLen = *(uint16_t*)p;
    uint16_t numLen = *(uint16_t*)(p + 2);
    uint16_t emailLen = *(uint16_t*)(p + 4);
    data.id = *(int*)(p + 6);
    data.sex = *(uint8_t*)(p + 10);

    data.name = (char*)(p + 11);

    data.number = (char*)(p + 12 + nameLen);

    data.email = (char*)(p + 13 + numLen + nameLen);
    return data;
}

enum Op {
    OP_EQ,  // =
    OP_GE,  // >
    OP_LE,  // <
};

enum ResFlag {
    RES_BAD,       // none
    RES_EQ_FIRST,  // a
    RES_HI,        // (-, a]
    RES_LO,        // [a, +)
    RES_EQ_BOTH,   // a or b
    RES_AND,       // [a, b]
    RES_OR,        // (-, a] or [b, +)
    RES_EQ_LO,     // a or [b, +)
    RES_EQ_HI,     // a or  (-, b]
    RES_ALL,       // all
};

static int str2int(const string& str, int i = 0) {
    int res = 0;
    while (str[i] < '0' || str[i] > '9')
        i++;
    while (str[i] >= '0' && str[i] <= '9') {
        res = res * 10 + str[i] - '0';
        i++;
    }
    return res;
}

const int INVALID_MARK = -98477777;

struct Condition {
    int a, b;
    ResFlag flag;
};

static Condition conditionParse(const string& sql, int start = 0) {
    int len = sql.length();
    Op op;  // first op
    Condition res;
    int num;  // first num
    start++;
    for (; start < len; start++) {
        if (sql[start - 1] == ' '&&  sql[start] == 'w'&&sql[start + 1] == 'h'&&sql[start + 2] == 'e'&&sql[start + 3] == 'r'&&sql[start + 4] == 'e' && sql[start + 5] == ' ')
        {
            start += 5;
            break;
        }
    }
    for (; start < len; start++) {
        if (sql[start] == '>') {
            op = OP_GE;
            num = str2int(sql, start) + (sql[start + 1] == '=' ? 0 : 1);
            res.a = num;
            res.flag = RES_LO;
            break;
        }
        else if (sql[start] == '=') {
            op = OP_EQ;
            num = str2int(sql, start);
            res.a = num;
            res.flag = RES_EQ_FIRST;
            break;
        }
        else if (sql[start] == '<') {
            op = OP_LE;
            num = str2int(sql, start) - (sql[start + 1] == '=' ? 0 : 1);
            res.a = num;
            res.flag = RES_HI;
            break;
        }
    }

    int conn = -1;  // 1 -> and, 0 -> or
    // find connective
    while (start < len && (sql[start] != 'a' && sql[start] != 'o'))
        start++;
    if (start < len) {
        if (sql[start] == 'a' && sql[start + 1] == 'n' && sql[start + 2] == 'd')
            conn = 1;
        else if (sql[start] == 'o' && sql[start + 1] == 'r')
            conn = 0;
    }

    if (conn != -1)
        for (; start < len; start++) {
            int x;
            if (sql[start] == '>') {
                x = str2int(sql, start) + (sql[start + 1] == '=' ? 0 : 1);
                if (op == OP_GE) {
                    if (conn == 1) {
                        // >=num and >=x
                        num = max(num, x);
                    }
                    else if (conn == 0) {
                        // >=num or >=x
                        num = min(num, x);
                    }
                    res.a = num;
                    res.flag = RES_LO;
                }
                else if (op == OP_EQ) {
                    if (conn == 1) {
                        // =num and >=x
                        if (num < x) {
                            res.flag = RES_BAD;
                        }
                        else {
                            res.a = num;
                            res.flag = RES_EQ_FIRST;
                        }
                    }
                    else if (conn == 0) {
                        // = num or >=x
                        if (num >= x) {
                            res.a = x;
                            res.flag = RES_LO;
                        }
                        else {
                            res.a = num;
                            res.b = x;
                            res.flag = RES_EQ_LO;
                        }
                    }
                }
                else if (op == OP_LE) {
                    if (conn == 1) {
                        // <=num and >=x
                        if (x > num) {
                            res.flag = RES_BAD;
                        }
                        else if (x == num) {
                            res.a = x;
                            res.flag = RES_EQ_FIRST;
                        }
                        else {
                            res.a = x;
                            res.b = num;
                            res.flag = RES_AND;
                        }
                    }
                    else if (conn == 0) {
                        // <=num or >=x
                        if (x <= num + 1) {
                            res.flag = RES_ALL;
                        }
                        else {
                            res.a = num;
                            res.b = x;
                            res.flag = RES_OR;
                        }
                    }
                }
                break;
            }
            else if (sql[start] == '=') {
                x = str2int(sql, start);
                if (op == OP_EQ) {
                    if (conn == 1) {  // and
                        if (x == num) {
                            res.a = x;
                            res.flag = RES_EQ_FIRST;
                        }
                        else {
                            res.flag = RES_BAD;
                        }
                    }
                    else if (conn == 0) {  // or
                        if (x == num) {
                            res.a = x;
                            res.flag = RES_EQ_FIRST;
                        }
                        else {
                            res.a = x;
                            res.b = num;
                            res.flag = RES_EQ_BOTH;
                        }
                    }
                }
                else if (op == OP_GE) {
                    if (conn == 1) {  // and
                        // >=num  and =x
                        if (num > x) {
                            res.flag = RES_BAD;
                        }
                        else {
                            res.a = x;
                            res.flag = RES_EQ_FIRST;
                        }
                    }
                    else if (conn == 0) {  // or
                                          // >=num or =x
                        if (x >= num) {
                            res.a = num;
                            res.flag = RES_LO;
                        }
                        else {
                            res.a = x;
                            res.b = num;
                            res.flag = RES_EQ_LO;
                        }
                    }
                }
                else if (op == OP_LE) {
                    if (conn == 1) {  // and
                        // <=num and =x
                        if (x > num) {
                            res.flag = RES_BAD;
                        }
                        else {
                            res.a = num;
                            res.flag = RES_EQ_FIRST;
                        }
                    }
                    else if (conn == 0) {  // or
                                          // <=num or =x
                        if (x > num) {
                            res.a = x;
                            res.b = num;
                            res.flag = RES_EQ_HI;
                        }
                        else {
                            res.a = num;
                            res.flag = RES_HI;
                        }
                    }
                }
                break;
            }
            else if (sql[start] == '<') {
                x = str2int(sql, start) - (sql[start + 1] == '=' ? 0 : 1);
                if (op == OP_GE) {
                    if (conn == 1) {  // and
                        // >=num and <=x
                        if (x < num) {
                            res.flag = RES_BAD;
                        }
                        else if (x == num) {
                            res.a = x;
                            res.flag = RES_EQ_FIRST;
                        }
                        else {
                            res.a = num;
                            res.b = x;
                            res.flag = RES_AND;
                        }
                    }
                    else if (conn == 0) {  // or
                                          // >=num or <=x
                        if (x + 1 >= num) {
                            res.flag = RES_ALL;
                        }
                        else {
                            res.a = x;
                            res.b = num;
                            res.flag = RES_OR;
                        }
                    }
                }
                else if (op == OP_EQ) {
                    if (conn == 1) {  // and
                        // =num and <=x
                        if (num > x) {
                            res.flag = RES_BAD;
                        }
                        else {
                            res.a = num;
                            res.flag = RES_EQ_FIRST;
                        }
                    }
                    else if (conn == 0) {  // or
                                          // =num OR <=x
                        if (num > x) {
                            res.a = num;
                            res.b = x;
                            res.flag = RES_EQ_HI;
                        }
                        else {
                            res.a = x;
                            res.flag = RES_HI;
                        }
                    }
                }
                else if (op == OP_LE) {
                    if (conn == 1) {  // and
                        // <= num and <= x
                        num = min(num, x);
                    }
                    else if (conn == 0) {  // or
                        num = max(num, x);
                    }
                    res.a = num;
                    res.flag = RES_HI;
                }
                break;
            }
        }
    return res;
}

static StorageManager s("table.db");
static StorageManager st("tree.db");
static BTree bTree(st);
static RecordBlock* block = NULL;
void initial() {}

void insert(RowData &data) {
    if (block == NULL) {
        if (s.getIndexOfRoot() == 0) {
            block = (RecordBlock*)s.getFreeBlock();
            block->init();
            s.setIndexOfRoot(block->header.index);
        }
        else
            block = (RecordBlock*)s.getBlock(s.getIndexOfRoot());
    }

    uint32_t pos = 0;
    Record* rcd = encode(data);
    if (!block->addRecord(rcd, &pos)) {
        RecordBlock* newBlock = (RecordBlock*)s.getFreeBlock();
        newBlock->init();

        block->header.next = newBlock->header.index;
        block = newBlock;

        s.setIndexOfRoot(block->header.index);

        if (!block->addRecord(rcd, &pos)) {
            // TODO:
        }
    }
    free(rcd);
    KeyValue kv ={data.id, block->header.index, pos};
    //kv.key = r.id, kv.value = block->header.index, kv.position = pos;
    bTree.insert(kv);

}

void insert(std::vector<RowData> rows) {
    //if (block == NULL) {
    //    if (s.getIndexOfRoot() == 0) {
    //        block = (RecordBlock*)s.getFreeBlock();
    //        block->init();
    //        s.setIndexOfRoot(block->header.index);
    //    }
    //    else
    //        block = (RecordBlock*)s.getBlock(s.getIndexOfRoot());
    //}
    for (auto& r : rows) {
        //if (bTree.search(r.id).value != 0) {
        //    continue;
        //}
        //uint32_t pos = 0;
        //Record* rcd = encode(r);
        //if (!block->addRecord(rcd, &pos)) {
        //    RecordBlock* newBlock = (RecordBlock*)s.getFreeBlock();
        //    newBlock->init();

        //    block->header.next = newBlock->header.index;
        //    block = newBlock;

        //    s.setIndexOfRoot(block->header.index);

        //    if (!block->addRecord(encode(r), &pos)) {
        //        // TODO:
        //    }
        //}
        //free(rcd);
        //KeyValue kv;
        //kv.key = r.id, kv.value = block->header.index, kv.position = pos;
        //bTree.insert(kv);
        insert(r);
    }
}

vector<KeyValue> search(const string& sql) {
    vector<KeyValue> r;
    vector<KeyValue> t;
    Condition con = conditionParse(sql);
    switch (con.flag) {
    case RES_ALL:
        r = bTree.values();
        break;
    case RES_AND:
        r = bTree.search(con.a, con.b);
        break;
    case RES_OR:
        r = bTree.lower(con.b);
        t = bTree.upper(con.a);
        for (auto& i : t)
            r.push_back(i);
        break;
    case RES_EQ_BOTH:
        if (con.a > con.b) {
            r.push_back(bTree.search(con.b));
            r.push_back(bTree.search(con.a));
        }
        else {
            r.push_back(bTree.search(con.a));
            r.push_back(bTree.search(con.b));
        }
        break;
    case RES_EQ_FIRST:
        r.push_back(bTree.search(con.a));
        break;
    case RES_EQ_HI:
        r = bTree.upper(con.b);
        r.push_back(bTree.search(con.a));
        break;
    case RES_EQ_LO:
        r = bTree.lower(con.b);
        //r.push_back(bTree.search(con.a));
        r.insert(r.begin(), bTree.search(con.a));
        break;
    case RES_HI:
        r = bTree.upper(con.a);
        break;
    case RES_LO:
        r = bTree.lower(con.a);
        break;
    default:
        break;
    }
    return r;
}

std::vector<RowData> query(string sql) {
    // select * from table where id >1500 and id < 1500;

    vector<RowData> res;
    auto r = search(sql);

    for (auto& i : r) {
        if (i.value == 0)
            continue;
        RecordBlock* rb = (RecordBlock*)s.getBlock(i.value);
        auto t = decode(rb->getRecord(i.position));
        res.push_back(t);
    }
    return res;
}

void del(string sql) {
    // delete from table where id > 300 or id > 784

    auto r = search(sql);
    for (auto& i : r) {
        // RecordBlock *rb = (RecordBlock*)s.getBlock(i.value);
        // rb->delRecord(i.position);
        if (i.value != 0)
            bTree.remove(i.key);
    }
}

void update(string sql) {
    // update table set number = 20157894 where id<900 or id>9950

    int sex = -1;
    string num = "", mail = "", name = "";

    int i;
    for (i = 0; i < sql.size(); i++)
        if (sql[i] == 's' && sql[i + 1] == 'e')
            break;
    i += 3;
    while (sql[i] == ' ')
        i++;
    if (sql[i] == 's') {
        // sex
        while (sql[i] != '=')
            i++;
        i++;
        sex = str2int(sql.substr(i));
    }
    else if (sql[i] == 'e') {
        // email
        while (sql[i] != '=')
            i++;
        i++;
        while (sql[i] == ' ')
            i++;
        int j = i + 1;
        while (sql[j] != ' ')
            j++;
        mail = sql.substr(i, j - i);
    }
    else if (sql[i] == 'n' && sql[i + 1] == 'a') {
        // name
        while (sql[i] != '=')
            i++;
        i++;
        while (sql[i] == ' ')
            i++;
        int j = i + 1;
        while (sql[j] != ' ')
            j++;
        name = sql.substr(i, j - i);
    }
    else if (sql[i] == 'n' && sql[i + 1] == 'u') {
        // number
        while (sql[i] != '=')
            i++;
        i++;
        while (sql[i] == ' ')
            i++;
        int j = i + 1;
        while (sql[j] != ' ')
            j++;
        num = sql.substr(i, j - i);
    }

    auto r = search(sql);

    for (auto& i : r) {
        if (i.value == 0)
            continue;
        RecordBlock* rb = (RecordBlock*)s.getBlock(i.value);
        auto t = decode(rb->getRecord(i.position));

        if (sex != -1) {
            t.sex = sex;
        }
        if (name != "") {
            t.name = name;
        }
        if (num != "") {
            t.number = num;
        }
        if (mail != "") {
            t.email = mail;
        }
        Record *r = encode(t);
        if (!rb->updateRecord(i.position, r)) {
            // 插入失败 空间不足
            insert(t);
        }
        free(r);
    }
}

void showRowData(const RowData& data) {
    std::cout << data.id << "\t" << data.sex << "\t" << data.name << "\t"
        << data.number << "\t" << data.email << std::endl;
}

void test() {
    initial();
    vector<RowData> data = {
        {12, 0, "Hello", "World", "hahha"},
        {123, 1, "Hello", "World", "hahha"},
        {2, 0, "Hello", "World", "hahha"},
        {1200, 1, "Hello", "World", "hahha"},

        {12, 0, "Hi", "World", "hahha"},
        {1273, 1, "jay", "World", "hahha"},
        {20, 0, "chou", "World", "hahha"},
        {120, 1, "ahah", "World", "hahha"},
    };

    insert(data);

    auto res = query("select * from table where id >= 0");
    std::cout << "----------\n";
    for (auto& i : res)
        showRowData(i);

    del("delete from table where id = 2");

    res = query("select * from table where id >= 0 and id >=120");
    std::cout << "----------\n";
    for (auto& i : res)
        showRowData(i);

    update("update table set number = 20157894 where id<900");

    res = query("select * from table where id >= 0");
    std::cout << "----------\n";
    for (auto& i : res)
        showRowData(i);
}

void test1() {
    for (int i = 0; i <= 8888; i++) {
        vector<RowData> rowDatas;
        RowData rowData;
        rowData.id = i;
        rowData.name = string("test").append(std::to_string(i));
        rowData.number = string("test").append(std::to_string(i));
        rowData.sex = true;
        rowData.email = string("test@test.com");
        rowDatas.push_back(rowData);
        insert(rowDatas);
    }

    vector<RowData> result =
        query("select * from table where id > 500 and id <999");
    bool flag = true;
    for (int i = 501; i < 999; i++) {  //保证数据顺序性
        RowData rowData = result.front();
        result.erase(result.begin());
        //RowData rowData = result[i - 501];
        if (rowData.id != i) {
            flag = false;
            break;
        }
        if (rowData.name.compare(string("test").append(std::to_string(i))) !=
            0) {
            flag = false;
            break;
        }
        if (rowData.number.compare(string("test").append(std::to_string(i))) !=
            0) {
            flag = false;
            break;
        }
        if (!rowData.sex) {
            flag = false;
            break;
        }
        if (rowData.email.compare(string("test@test.com")) != 0) {
            flag = false;
            break;
        }
    }
    // 0~8888
    del("delete from table where id < 300 or id > 7999");
    // 300 ~ 8000
    result =
        query("select * from table where id < 300 or id >7999");
    if (result.size() != 0) flag = false;

    update("update table set email = yangtao@mail.com where id=900 or id>=7000");

    result = query("select * from table where id=900 or id>=7000");

    for (RowData &data : result) {
        if (data.email.compare(string("yangtao@mail.com")) != 0) {
            flag = false;
            break;
        }
    }
    

    if (flag) {
        std::cout << "Yes!!!\n";
    }
    else {
        std::cout << "NO???\n";
    }
}

// select * from table where id = 500 and id >     999
// select * from table where id = 500 and id =     999
// select * from table where id> 500 and id <999
// select * from table where id > 500 and id >999
// select * from table where id < 500 and id <999

// select * from table where id = 500 or id =     999
// select * from table where id> 500 or id <999
// select * from table where id > 500 or id >999
// select * from table where id < 500 or id <999

#include <assert.h>
void test_condition() {
    auto res =
        conditionParse("select * from table where id = 500 and id >     999");
    assert(res.flag == RES_BAD);

    res =
        conditionParse("select * from table where id >     999 and id = 500");
    assert(res.flag == RES_BAD);

    res = conditionParse("select * from table where id =     999 and id = 500");
    assert(res.flag == RES_BAD);
    res = conditionParse("select * from table where id = 500 or id =     999");
    assert(res.flag == RES_EQ_BOTH && (res.a == 500 && res.b == 999 || res.b == 500 && res.a == 999));
    res = conditionParse("select * from table where id = 500 or id =  500");
    assert(res.flag == RES_EQ_FIRST && res.a == 500);
    res = conditionParse("select * from table where id = 500 and id =  500");
    assert(res.flag == RES_EQ_FIRST && res.a == 500);

    res = conditionParse("select * from table where id >=  500 and id = 500");
    assert(res.flag == RES_EQ_FIRST && res.a == 500);

    res = conditionParse("select * from table where id <=  500 and id > 500");
    assert(res.flag == RES_BAD);
    res = conditionParse("select * from table where id <=  500 or id > 500 ");
    assert(res.flag == RES_ALL);

    res = conditionParse("select * from table where id =  500 or id < 400");
    assert(res.flag == RES_EQ_HI && res.a == 500 && res.b == 399);

    res = conditionParse("select * from table where id =  500 or id >= 900 ");
    assert(res.flag == RES_EQ_LO && res.a == 500 && res.b == 900);

    res = conditionParse("select * from table where id < 200");
    assert(res.flag == RES_HI && res.a == 199);

    res = conditionParse("select * from table where id >=20 and id <= 200 ");
    assert(res.flag == RES_AND && res.a == 20 && res.b == 200);

    res = conditionParse("select * from table where id >=200 or id <= 20 ");
    assert(res.flag == RES_OR && res.a == 20 && res.b == 200);
}