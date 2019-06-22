#include <assert.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include "block.h"
#include "btree.h"
#include "buffer.h"
#include "storage.h"
using std::max;
using std::min;

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
    Record* r = (Record*)malloc(16 + nameLen + emailLen + numLen);
    r->size = 16 + nameLen + emailLen + numLen;
    uint8_t* p = (uint8_t*)r->data;
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
        if (sql[start - 1] == ' ' && sql[start] == 'w' &&
            sql[start + 1] == 'h' && sql[start + 2] == 'e' &&
            sql[start + 3] == 'r' && sql[start + 4] == 'e' &&
            sql[start + 5] == ' ') {
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
        } else if (sql[start] == '=') {
            op = OP_EQ;
            num = str2int(sql, start);
            res.a = num;
            res.flag = RES_EQ_FIRST;
            break;
        } else if (sql[start] == '<') {
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
                    } else if (conn == 0) {
                        // >=num or >=x
                        num = min(num, x);
                    }
                    res.a = num;
                    res.flag = RES_LO;
                } else if (op == OP_EQ) {
                    if (conn == 1) {
                        // =num and >=x
                        if (num < x) {
                            res.flag = RES_BAD;
                        } else {
                            res.a = num;
                            res.flag = RES_EQ_FIRST;
                        }
                    } else if (conn == 0) {
                        // = num or >=x
                        if (num >= x) {
                            res.a = x;
                            res.flag = RES_LO;
                        } else {
                            res.a = num;
                            res.b = x;
                            res.flag = RES_EQ_LO;
                        }
                    }
                } else if (op == OP_LE) {
                    if (conn == 1) {
                        // <=num and >=x
                        if (x > num) {
                            res.flag = RES_BAD;
                        } else if (x == num) {
                            res.a = x;
                            res.flag = RES_EQ_FIRST;
                        } else {
                            res.a = x;
                            res.b = num;
                            res.flag = RES_AND;
                        }
                    } else if (conn == 0) {
                        // <=num or >=x
                        if (x <= num + 1) {
                            res.flag = RES_ALL;
                        } else {
                            res.a = num;
                            res.b = x;
                            res.flag = RES_OR;
                        }
                    }
                }
                break;
            } else if (sql[start] == '=') {
                x = str2int(sql, start);
                if (op == OP_EQ) {
                    if (conn == 1) {  // and
                        if (x == num) {
                            res.a = x;
                            res.flag = RES_EQ_FIRST;
                        } else {
                            res.flag = RES_BAD;
                        }
                    } else if (conn == 0) {  // or
                        if (x == num) {
                            res.a = x;
                            res.flag = RES_EQ_FIRST;
                        } else {
                            res.a = x;
                            res.b = num;
                            res.flag = RES_EQ_BOTH;
                        }
                    }
                } else if (op == OP_GE) {
                    if (conn == 1) {  // and
                        // >=num  and =x
                        if (num > x) {
                            res.flag = RES_BAD;
                        } else {
                            res.a = x;
                            res.flag = RES_EQ_FIRST;
                        }
                    } else if (conn == 0) {  // or
                                             // >=num or =x
                        if (x >= num) {
                            res.a = num;
                            res.flag = RES_LO;
                        } else {
                            res.a = x;
                            res.b = num;
                            res.flag = RES_EQ_LO;
                        }
                    }
                } else if (op == OP_LE) {
                    if (conn == 1) {  // and
                        // <=num and =x
                        if (x > num) {
                            res.flag = RES_BAD;
                        } else {
                            res.a = num;
                            res.flag = RES_EQ_FIRST;
                        }
                    } else if (conn == 0) {  // or
                                             // <=num or =x
                        if (x > num) {
                            res.a = x;
                            res.b = num;
                            res.flag = RES_EQ_HI;
                        } else {
                            res.a = num;
                            res.flag = RES_HI;
                        }
                    }
                }
                break;
            } else if (sql[start] == '<') {
                x = str2int(sql, start) - (sql[start + 1] == '=' ? 0 : 1);
                if (op == OP_GE) {
                    if (conn == 1) {  // and
                        // >=num and <=x
                        if (x < num) {
                            res.flag = RES_BAD;
                        } else if (x == num) {
                            res.a = x;
                            res.flag = RES_EQ_FIRST;
                        } else {
                            res.a = num;
                            res.b = x;
                            res.flag = RES_AND;
                        }
                    } else if (conn == 0) {  // or
                                             // >=num or <=x
                        if (x + 1 >= num) {
                            res.flag = RES_ALL;
                        } else {
                            res.a = x;
                            res.b = num;
                            res.flag = RES_OR;
                        }
                    }
                } else if (op == OP_EQ) {
                    if (conn == 1) {  // and
                        // =num and <=x
                        if (num > x) {
                            res.flag = RES_BAD;
                        } else {
                            res.a = num;
                            res.flag = RES_EQ_FIRST;
                        }
                    } else if (conn == 0) {  // or
                                             // =num OR <=x
                        if (num > x) {
                            res.a = num;
                            res.b = x;
                            res.flag = RES_EQ_HI;
                        } else {
                            res.a = x;
                            res.flag = RES_HI;
                        }
                    }
                } else if (op == OP_LE) {
                    if (conn == 1) {  // and
                        // <= num and <= x
                        num = min(num, x);
                    } else if (conn == 0) {  // or
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

uint8_t keylen = sizeof(int), vallen = sizeof(RecordManager::Location);
int cmp(const void* a, const void* b) {
    return *(int*)a - *(int*)b;
}
StorageManager s("table.db");
StorageManager st("index.db");
BTree bTree(keylen, vallen, cmp, st);
RecordManager rm(s);

void initial() {
    s.create("table.db");
    s.open("table.db");
    st.create("index.db");
    st.open("index.db");
}

void insert(RowData& data) {
    Record* rcd = encode(data);
    RecordManager::Location loc;
    rm.put(rcd, &loc);
    free(rcd);
    bTree.put(&data.id, &loc);
}

void insert(std::vector<RowData> rows) {
    for (auto& r : rows) {
        insert(r);
    }
}

// vector<RecordManager::Location> search(const string& sql) {
// vector<RecordManager::Location> r;

// Condition con = conditionParse(sql);
// BTree::Iterator* it = bTree.iterator();
// int t;
// switch (con.flag) {
//     case RES_ALL:
//         it->open(NULL, NULL);
//         do {
//             RecordManager::Location loc;
//             it->get(NULL, &loc);
//             r.push_back(loc);
//         } while (it->next());
//         it->close();
//         break;
//     case RES_AND:
//         t = con.b + 1;
//         it->open(&con.a, &t);
//         do {
//             RecordManager::Location loc;
//             it->get(NULL, &loc);
//             r.push_back(loc);
//         } while (it->next());
//         it->close();
//         break;
//     case RES_OR:
//         t = con.a + 1;
//         it->open(NULL, &t);
//         do {
//             RecordManager::Location loc;
//             it->get(NULL, &loc);
//             r.push_back(loc);
//         } while (it->next());
//         it->close();
//         it->open(&con.b, NULL);
//         do {
//             RecordManager::Location loc;
//             it->get(NULL, &loc);
//             r.push_back(loc);
//         } while (it->next());
//         it->close();
//     case RES_EQ_BOTH:
//         RecordManager::Location loca, locb;
//         bTree.get(&con.a, &loca);
//         bTree.get(&con.b, &locb);
//         if (con.a > con.b) {
//             r.push_back(loca);
//             r.push_back(locb);
//         } else {
//             r.push_back(locb);
//             r.push_back(loca);
//         }
//         break;
//     case RES_EQ_FIRST:
//         RecordManager::Location loca;
//         bTree.get(&con.a, &loca);
//         r.push_back(loca);
//         break;
//     case RES_EQ_HI:
//         t = con.b + 1;
//         it->open(NULL, &t);
//         do {
//             RecordManager::Location loc;
//             it->get(NULL, &loc);
//             r.push_back(loc);
//         } while (it->next());
//         it->close();

//         RecordManager::Location loca;
//         bTree.get(&con.a, &loca);
//         r.push_back(loca);
//         break;
//     case RES_EQ_LO:
//         RecordManager::Location loca;
//         bTree.get(&con.a, &loca);
//         r.push_back(loca);

//         it->open(&con.b, NULL);
//         do {
//             RecordManager::Location loc;
//             it->get(NULL, &loc);
//             r.push_back(loc);
//         } while (it->next());
//         it->close();
//         break;
//     case RES_HI:
//         t = con.a + 1;
//         it->open(NULL, &t);
//         do {
//             RecordManager::Location loc;
//             it->get(NULL, &loc);
//             r.push_back(loc);
//         } while (it->next());
//         it->close();
//         break;
//     case RES_LO:
//         it->open(&con.a, NULL);
//         do {
//             RecordManager::Location loc;
//             it->get(NULL, &loc);
//             r.push_back(loc);
//         } while (it->next());
//         it->close();
//         break;
//     default:
//         break;
// }
// delete it;
// }

std::vector<RowData> query(string sql) {
    vector<RowData> res;
    vector<RecordManager::Location> r;

    Condition con = conditionParse(sql);
    BTree::Iterator* it = bTree.iterator();
    int t;
    switch (con.flag) {
        case RES_AND:
            t = con.b + 1;
            it->open(&con.a, &t);
            do {
                RecordManager::Location loc;
                it->get(NULL, &loc);
                const Record* rcd = rm.get(&loc);
                res.push_back(decode(rcd));
            } while (it->next());
            it->close();
            break;
        case RES_OR:
            t = con.a + 1;
            it->open(NULL, &t);
            do {
                RecordManager::Location loc;
                it->get(NULL, &loc);
                const Record* rcd = rm.get(&loc);
                res.push_back(decode(rcd));
            } while (it->next());
            it->close();
            it->open(&con.b, NULL);
            do {
                RecordManager::Location loc;
                it->get(NULL, &loc);
                const Record* rcd = rm.get(&loc);
                res.push_back(decode(rcd));
            } while (it->next());
            it->close();
        case RES_EQ_FIRST:
            RecordManager::Location loca;
            bTree.get(&con.a, &loca);
            const Record* rcd = rm.get(&loca);
            res.push_back(decode(rcd));
            break;

        case RES_HI:
            t = con.a + 1;
            it->open(NULL, &t);
            do {
                RecordManager::Location loc;
                it->get(NULL, &loc);
                const Record* rcd = rm.get(&loc);
                res.push_back(decode(rcd));
            } while (it->next());
            it->close();
            break;
        case RES_LO:
            it->open(&con.a, NULL);
            do {
                RecordManager::Location loc;
                it->get(NULL, &loc);
                const Record* rcd = rm.get(&loc);
                res.push_back(decode(rcd));
            } while (it->next());
            it->close();
            break;
        default:
            break;
    }
    delete it;

    return res;
}

void del(string sql) {
    Condition con = conditionParse(sql);
    BTree::Iterator* it = bTree.iterator();
    int t;
    switch (con.flag) {
        case RES_AND:
            t = con.b + 1;
            it->open(&con.a, &t);
            do {
                it->remove();
            } while (it->next());
            it->close();
            break;
        case RES_OR:
            t = con.a + 1;
            it->open(NULL, &t);
            do {
                it->remove();
            } while (it->next());
            it->close();
            it->open(&con.b, NULL);
            do {
                it->remove();
            } while (it->next());
            it->close();
        case RES_EQ_FIRST:
            RecordManager::Location loca;
            bTree.remove(&con.a);
            break;

        case RES_HI:
            t = con.a + 1;
            it->open(NULL, &t);
            do {
                it->remove();
            } while (it->next());
            it->close();
            break;
        case RES_LO:
            it->open(&con.a, NULL);
            do {
                it->remove();
            } while (it->next());
            it->close();
            break;
        default:
            break;
    }
    delete it;
}

Record* up(const Record* rcd, int sex, string name, string num, string mail) {
    RowData t = decode(rcd);

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
    return encode(t);
}

void update(string sql) {
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
    } else if (sql[i] == 'e') {
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
    } else if (sql[i] == 'n' && sql[i + 1] == 'a') {
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
    } else if (sql[i] == 'n' && sql[i + 1] == 'u') {
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

    Condition con = conditionParse(sql);
    BTree::Iterator* it = bTree.iterator();
    int t;
    switch (con.flag) {
        case RES_AND:
            t = con.b + 1;
            it->open(&con.a, &t);
            do {
                RecordManager::Location loc;
                it->get(NULL, &loc);
                const Record* rcd = rm.get(&loc);
                // update
                Record* r = up(rcd, sex, name, num, mail);
                rm.set(r, &loc);
                it->set(&loc);
            } while (it->next());
            it->close();
            break;
        case RES_OR:
            t = con.a + 1;
            it->open(NULL, &t);
            do {
                RecordManager::Location loc;
                it->get(NULL, &loc);
                const Record* rcd = rm.get(&loc);
                // update
                Record* r = up(rcd, sex, name, num, mail);
                rm.set(r, &loc);
                it->set(&loc);
            } while (it->next());
            it->close();
            it->open(&con.b, NULL);
            do {
                RecordManager::Location loc;
                it->get(NULL, &loc);
                const Record* rcd = rm.get(&loc);
                // update
                Record* r = up(rcd, sex, name, num, mail);
                rm.set(r, &loc);
                it->set(&loc);
            } while (it->next());
            it->close();
        case RES_EQ_FIRST:
            RecordManager::Location loca;
            bTree.get(&con.a, &loca);
            const Record* rcd = rm.get(&loca);
            Record* r = up(rcd, sex, name, num, mail);
            bTree.put(&con.a, &loca);
            break;

        case RES_HI:
            t = con.a + 1;
            it->open(NULL, &t);
            do {
                RecordManager::Location loc;
                it->get(NULL, &loc);
                const Record* rcd = rm.get(&loc);
                // update
                Record* r = up(rcd, sex, name, num, mail);
                rm.set(r, &loc);
                it->set(&loc);
            } while (it->next());
            it->close();
            break;
        case RES_LO:
            it->open(&con.a, NULL);
            do {
                RecordManager::Location loc;
                it->get(NULL, &loc);
                const Record* rcd = rm.get(&loc);
                // update
                Record* r = up(rcd, sex, name, num, mail);
                rm.set(r, &loc);
                it->set(&loc);
            } while (it->next());
            it->close();
            break;
        default:
            break;
    }
    delete it;
}
