#pragma once

#include <assert.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include "block.h"
#include "btree.h"

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

void *encode(const RowData &data) {
  u16 nameLen = data.name.size();
  u16 numLen = data.number.size();
  u16 emailLen = data.email.size();
  void *r = malloc(16 + nameLen + emailLen + numLen);

  u8 *p = (u8 *)r;

  *(u16 *)p = 16 + nameLen + emailLen + numLen;
  p += sizeof(u16);

  *(u16 *)p = nameLen;
  p += 2;
  *(u16 *)p = numLen;
  p += 2;
  *(u16 *)p = emailLen;

  p += 2;
  *(int *)p = data.id;
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

RowData decode(const void *record) {
  RowData data;
  const u8 *p = (u8 *)record + sizeof(u16);
  u16 nameLen = *(u16 *)p;
  u16 numLen = *(u16 *)(p + 2);
  u16 emailLen = *(u16 *)(p + 4);
  data.id = *(int *)(p + 6);
  data.sex = *(u8 *)(p + 10);

  data.name = (char *)(p + 11);

  data.number = (char *)(p + 12 + nameLen);

  data.email = (char *)(p + 13 + numLen + nameLen);
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

static int str2int(const string &str, int i = 0) {
  int res = 0;
  while (str[i] < '0' || str[i] > '9') i++;
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

static Condition conditionParse(const string &sql, int start = 0) {
  int len = sql.length();
  Op op;  // first op
  Condition res;
  int num;  // first num
  start++;
  for (; start < len; start++) {
    if (sql[start - 1] == ' ' && sql[start] == 'w' && sql[start + 1] == 'h' &&
        sql[start + 2] == 'e' && sql[start + 3] == 'r' &&
        sql[start + 4] == 'e' && sql[start + 5] == ' ') {
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
  while (start < len && (sql[start] != 'a' && sql[start] != 'o')) start++;
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

int cmp(const void *a, const void *b, void *) { return *(int *)a - *(int *)b; }

BTree *bTree = NULL;
bool initialized = false;

void release() { delete bTree; }

void init() {
  if (initialized) return;
  initialized = true;
  bTree = new BTree(sizeof(int), cmp);
  bTree->open("table.db");
  atexit(release);
}

void initial() {
  initialized = true;
  bTree = new BTree(sizeof(int), cmp);
  bTree->create("table.db");
  bTree->open("table.db");
  atexit(release);
}

void insert(RowData &data) {
  void *rcd = encode(data);
  if (!bTree->put(&data.id, rcd, *(u16 *)rcd))
    fprintf(stderr, "Error: %s\n", " btree put");
  free(rcd);
}

void insert(std::vector<RowData> rows) {
  init();
  for (auto &r : rows) {
    insert(r);
  }
}

std::vector<RowData> query(string sql) {
  init();
  vector<RowData> res;
  // vector<RecordManager::Location> r;

  Condition con = conditionParse(sql);
  BTree::Iterator it = bTree->iterator();
  switch (con.flag) {
    case RES_AND:
      if (it.locate(&con.a, 1)) {
        do {
          RowData d = decode(it.getValue());
          if (d.id > con.b) {
            break;
          }
          res.push_back(d);
        } while (it.next());
      }
      break;
    case RES_OR:
      //            (-, a], [b, +)
      if (it.first()) {
        do {
          RowData d = decode(it.getValue());
          if (d.id > con.a) {
            break;
          }
          res.push_back(d);
        } while (it.next());
      }

      if (it.locate(&con.b, 1)) {
        do {
          RowData d = decode(it.getValue());
          res.push_back(d);
        } while (it.next());
      }
      break;
    case RES_EQ_FIRST:
      if (it.locate(&con.a, 1)) {
        RowData d = decode(it.getValue());
        res.push_back(d);
      }
      break;
    case RES_HI:
      //            (-, a]
      if (it.first()) {
        do {
          RowData d = decode(it.getValue());
          if (d.id > con.a) {
            break;
          }
          res.push_back(d);
        } while (it.next());
      }
      break;
    case RES_LO:
      if (it.locate(&con.a, 1)) {
        do {
          RowData d = decode(it.getValue());
          res.push_back(d);
        } while (it.next());
      }
      break;
    default:
      fprintf(stderr, "Error: %s\n", " valid condition in query");
      break;
  }
  return res;
}

void del(string sql) {
  init();
  Condition con = conditionParse(sql);
  BTree::Iterator it = bTree->iterator();
  switch (con.flag) {
    case RES_AND:
      if (it.locate(&con.a, 1)) {
        do {
          RowData d = decode(it.getValue());
          if (d.id > con.b) {
            break;
          }
          it.remove();
        } while (it.next());
      }
      break;
    case RES_OR:
      //            (-, a], [b, +)
      if (it.first()) {
        do {
          RowData d = decode(it.getValue());
          if (d.id > con.a) {
            break;
          }
          it.remove();
        } while (it.next());
      }

      if (it.locate(&con.b, 1)) {
        do {
          RowData d = decode(it.getValue());
          it.remove();
        } while (it.next());
      }
      break;
    case RES_EQ_FIRST:
      if (it.locate(&con.a, 1)) {
        RowData d = decode(it.getValue());
        it.remove();
      }
      break;
    case RES_HI:
      //            (-, a]
      if (it.first()) {
        do {
          RowData d = decode(it.getValue());
          if (d.id > con.a) {
            break;
          }
          it.remove();
        } while (it.next());
      }
      break;
    case RES_LO:
      if (it.locate(&con.a, 1)) {
        do {
          RowData d = decode(it.getValue());
          it.remove();
        } while (it.next());
      }
      break;
    default:
      fprintf(stderr, "Error: %s\n", " valid condition in delete");
      break;
  }
}

void *up(RowData t, int sex, string name, string num, string mail) {
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
  init();
  int sex = -1;
  string num = "", mail = "", name = "";

  int i;

  for (i = 0; i < sql.size(); i++)
    if (sql[i] == 's' && sql[i + 1] == 'e') break;
  i += 3;
  while (sql[i] == ' ') i++;
  if (sql[i] == 's') {
    // sex
    while (sql[i] != '=') i++;
    i++;
    sex = str2int(sql.substr(i));
  } else if (sql[i] == 'e') {
    // email
    while (sql[i] != '=') i++;
    i++;
    while (sql[i] == ' ') i++;
    int j = i + 1;
    while (sql[j] != ' ') j++;
    mail = sql.substr(i, j - i);
  } else if (sql[i] == 'n' && sql[i + 1] == 'a') {
    // name
    while (sql[i] != '=') i++;
    i++;
    while (sql[i] == ' ') i++;
    int j = i + 1;
    while (sql[j] != ' ') j++;
    name = sql.substr(i, j - i);
  } else if (sql[i] == 'n' && sql[i + 1] == 'u') {
    // number
    while (sql[i] != '=') i++;
    i++;
    while (sql[i] == ' ') i++;
    int j = i + 1;
    while (sql[j] != ' ') j++;
    num = sql.substr(i, j - i);
  }

  Condition con = conditionParse(sql);
  BTree::Iterator it = bTree->iterator();

  switch (con.flag) {
    case RES_AND:
      if (it.locate(&con.a, 1)) {
        do {
          RowData d = decode(it.getValue());
          if (d.id > con.b) {
            break;
          }
          it.remove();
          void *t = up(d, sex, name, num, mail);
          bTree->put(&d.id, t, *(u16 *)t);
        } while (it.next());
      }
      break;
    case RES_OR:
      //            (-, a], [b, +)
      if (it.first()) {
        do {
          RowData d = decode(it.getValue());
          if (d.id > con.a) {
            break;
          }
          it.remove();
          void *t = up(d, sex, name, num, mail);
          bTree->put(&d.id, t, *(u16 *)t);
        } while (it.next());
      }

      if (it.locate(&con.b, 1)) {
        do {
          RowData d = decode(it.getValue());
          it.remove();
          void *t = up(d, sex, name, num, mail);
          bTree->put(&d.id, t, *(u16 *)t);
        } while (it.next());
      }
      break;
    case RES_EQ_FIRST:
      if (it.locate(&con.a, 1)) {
        RowData d = decode(it.getValue());
        it.remove();
        void *t = up(d, sex, name, num, mail);
        bTree->put(&d.id, t, *(u16 *)t);
      }
      break;
    case RES_HI:
      //            (-, a]
      if (it.first()) {
        do {
          RowData d = decode(it.getValue());
          if (d.id > con.a) {
            break;
          }
          it.remove();
          void *t = up(d, sex, name, num, mail);
          bTree->put(&d.id, t, *(u16 *)t);
        } while (it.next());
      }
      break;
    case RES_LO:
      if (it.locate(&con.a, 1)) {
        do {
          RowData d = decode(it.getValue());
          it.remove();
          void *t = up(d, sex, name, num, mail);
          bTree->put(&d.id, t, *(u16 *)t);
        } while (it.next());
      }
      break;
    default:
      fprintf(stderr, "Error: %s\n", " valid condition in update");
      break;
  }
}

#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

int cmd = 0;

bool CheckFile(string fileName) {
  ifstream file(fileName, ios::binary | ios::in);
  if (!file.is_open()) return false;
  file.seekg(0, ios::beg);
  char result[4] = {0xc1, 0xc6, 0xf0, 0x1e};
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

  // if (!CheckFile("table.db")) {
  //    printf("wrong\n");
  //    return;
  //}
  vector<RowData> result =
      query("select * from table where id > 500 and id <999");
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
    if (rowData.email.compare(
            string("20190603@").append(to_string(i)).append(".com")) != 0) {
      flag = false;
      break;
    }
  }
  // del("delete from table where id > 2956 and id < 7854");
  if (flag == false) {
    printf("wrong\n");
  } else
    printf("yes\n");
}

void testM3() {
  update("update table set number = 20190603x where id<50 or id>9950");
  vector<RowData> rowDatas =
      query("select * from table where id<50 or id>9950");
  if (rowDatas.size() != 99) {
    printf("wrong\n");
    return;
  }
  for (vector<RowData>::iterator it = rowDatas.begin(); it != rowDatas.end();
       it++) {
    if ((*it).number.compare(string("20190603x")) != 0) {
      printf("wrong\n");
      return;
    }
  }
  //if (!CheckFile("table.db")) {
  //  printf("wrong\n");
  //  return;
  //}
  printf("yes\n");
  return;
}

void testM2() {
  int flag = true;
  del("delete from table where id > 2956 and id < 7854");
  vector<RowData> result =
      query("select * from table where id > 2900 and id <5000");
  for (int i = 2901; i < 2957; i++)
  // for (int i = 2991; i < 2957; i++)
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
    if (rowData.email.compare(
            string("20190603@").append(to_string(i)).append(".com")) != 0) {
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
  // for (int i = 7855; i < 7900; i++)
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
    if (rowData.email.compare(
            string("20190603@").append(to_string(i)).append(".com")) != 0) {
      flag = false;
      break;
    }
  }
  if (flag == false) {
    printf("wrong\n");
    return;
  }

  // if (!CheckFile("table.db")) {
  //  printf("wrong\n");
  //  return;
  //}
  printf("yes\n");
}

void test() {
  initial();
  vector<RowData> rowDatas;
  for (int i = 0; i < 10000; i++) {
    RowData rowData;
    rowData.id = i;
    rowData.name = string("ccg").append(to_string(i));
    rowData.number = string("20190603").append(to_string(i));
    rowData.sex = true;
    rowData.email = string("20190603@").append(to_string(i)).append(".com");
    rowDatas.push_back(rowData);
  }
  insert(rowDatas);

  vector<RowData> result =
      query("select * from table where id <5000 and id>200");
  bool flag = true;
  for (int i = 201; i < 5000; i++) {
    RowData rowData = result[i - 201];
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
    if (rowData.email.compare(
            string("20190603@").append(to_string(i)).append(".com")) != 0) {
      flag = false;
      break;
    }
  }
  result = query("select * from table where id <500 or id>10000");

  // del("delete from table where id > 2956 and id < 7854");
  if (flag == false)
    printf("wrong\n");
  else
    printf("yes\n");
}
