import pymysql
import random
import string
import time
import functools
import inspect


def line():
    return inspect.currentframe().f_back.f_lineno


def log(func):
    @functools.wraps(func)
    def wrapper(*args, **kw):
        print('start executing %s()' % func.__name__)
        s = time.perf_counter()
        res = func(*args, **kw)
        elapsed = time.perf_counter() - s
        print('%s() executed for %fs' % (func.__name__, elapsed))
        return res
    return wrapper


def random_string(len):
    letters = string.ascii_letters
    return ''.join([random.choice(letters) for i in range(random.randint(5, len))])


@log
def generate_data(data_len: int):
    s = set()
    data = []
    for _ in range(data_len):
        _id = random.randint(0, data_len*10)
        while _id in s:
            _id = random.randint(0, data_len*10)
        s.add(_id)
        age = random.randint(0, 100)
        name = random_string(20)
        data.append((_id, age, name))
    return data


@log
def connect():
    return pymysql.connect(
        host='localhost',
        port=3306,
        user='root',
        password='hello',
        database='test'
    )


@log
def create(con: pymysql.Connection):
    cur = con.cursor()
    cur.execute('drop table if exists user')
    cur.execute(
        '''create table user (
            id int primary key,
            age int,
            name varchar(20),
            key(age)
        ) engine = example''')
    cur.close()


@log
def insert(con: pymysql.Connection, data: list):
    cur = con.cursor()
    try:
        # cur.executemany('insert into user values (%s, %s, "%s")', data)
        # # cur.executemany('insert into user values (%s, %s, "%s")', data)
        for u in data:
            cur.execute('insert into user value (%s, %s, "%s")' % u)
            assert 1 == cur.rowcount, 'insert error'
        #
    except Exception as e:
        print('insert error: ', e)
    finally:
        cur.close()


@log
def query(con: pymysql.Connection, data: list):
    data.sort()
    cur = con.cursor()
    try:
        cur.execute('select * from user')
        rows = cur.fetchall()
        rows = [i for i in rows]
        rows.sort()
        print("query rows len: %d" % len(rows))
        assert rows == data, 'query error: the result of query is not equal to the data inserted'
    except Exception as e:
        print('query error: ', e)
    finally:
        cur.close()


@log
def condition_query(con: pymysql.Connection, data: list):
    test_cnt_each_case = 5
    cur = con.cursor()
    data.sort()
    # id 0 ~ 10 * len(data)
    # start id
    # start id >
    for _ in range(test_cnt_each_case):
        a = random.randint(0, 10*len(data))
        except_res = [i for i in data if i[0] > a]
        try:
            cur.execute('select * from user where id > %d' % a)
            res = cur.fetchall()
            res = [i for i in res]
            assert res == except_res, 'the result of query is not expected'
        except Exception as e:
            print('error: ', e)
    # end id >

    # start id >=
    for _ in range(test_cnt_each_case):
        a = random.randint(0, 10*len(data))
        except_res = [i for i in data if i[0] >= a]
        try:
            cur.execute('select * from user where id >= %d' % a)
            res = cur.fetchall()
            res = [i for i in res]
            # res.sort()
            assert res == except_res, 'the result of query is not expected'
        except Exception as e:
            print('error: ', e)
    # end id >=

    # start id < order by id desc
    for _ in range(test_cnt_each_case):
        a = random.randint(0, 10*len(data))
        except_res = [i for i in data if i[0] < a]
        except_res.sort(reverse=True)
        try:
            cur.execute(
                'select * from user where id < %d order by id desc' % a)
            res = cur.fetchall()
            res = [i for i in res]
            assert res == except_res, 'the result of query is not expected'
        except Exception as e:
            print('error: ', e)
    # end id < order by id desc

    # start id <= order by id desc
    for _ in range(test_cnt_each_case):
        a = random.randint(0, 10*len(data))
        except_res = [i for i in data if i[0] <= a]
        except_res.sort(reverse=True)
        try:
            cur.execute(
                'select * from user where id <= %d order by id desc' % a)
            res = cur.fetchall()
            res = [i for i in res]
            assert res == except_res, 'the result of query is not expected'
        except Exception as e:
            print('error: ', e)
    # end id <= order by id desc
    # end id
    print('condition: age')

    def cmp_age(a, b):
        if a[1] == b[1]:
            return a[0]-b[0]
        return a[1]-b[1]
    # start age
    # start age >
    for _ in range(test_cnt_each_case):
        a = random.randint(0, 100)
        except_res = [i for i in data if i[1] > a]
        except_res.sort(key=functools.cmp_to_key(cmp_age))
        try:
            cur.execute('select * from user where age > %d' % a)
            res = cur.fetchall()
            res = [i for i in res]
            # res.sort(key=functools.cmp_to_key(cmp_age))
            assert res == except_res, 'the result of query is not expected %s' % line()
        except Exception as e:
            print('error: ', e)
    # end age >

    # start age >=
    for _ in range(test_cnt_each_case):
        a = random.randint(0, 100)
        except_res = [i for i in data if i[1] >= a]
        except_res.sort(key=functools.cmp_to_key(cmp_age))
        try:
            cur.execute('select * from user where age >= %d' % a)
            res = cur.fetchall()
            res = [i for i in res]
            # res.sort(key=functools.cmp_to_key(cmp_age))
            assert res == except_res, 'the result of query is not expected %s' % line()
        except Exception as e:
            print('error: ', e)
    # end age >=

    # start age < order by id desc
    for _ in range(test_cnt_each_case):
        a = random.randint(0, 100)
        except_res = [i for i in data if i[1] < a]
        except_res.sort(reverse=True, key=functools.cmp_to_key(cmp_age))
        try:
            cur.execute(
                'select * from user where age < %d order by age desc' % a)
            res = cur.fetchall()
            res = [i for i in res]
            # res.sort(reverse=True, key=functools.cmp_to_key(cmp_age))
            assert res == except_res, 'the result of query is not expected %s' % line()
        except Exception as e:
            print('error: ', e)
    # end age < order by id desc

    # start age <= order by id desc
    for _ in range(test_cnt_each_case):
        a = random.randint(0, 100)
        except_res = [i for i in data if i[1] <= a]

        except_res.sort(reverse=True, key=functools.cmp_to_key(cmp_age))
        try:
            cur.execute(
                'select * from user where age <= %d order by age desc' % a)
            res = cur.fetchall()
            res = [i for i in res]
            # res.sort(reverse=True, key=functools.cmp_to_key(cmp_age))
            assert res == except_res, 'the result of query is not expected %s' % line()
        except Exception as e:
            print('error: ', e)
    # end age <= order by age desc
    # end age
    cur.close()


@log
def update(con: pymysql.Connection, data: list):
    test_cnt_each_case = 2
    cur = con.cursor()
    # update name
    print('update by id')
    for _ in range(test_cnt_each_case):
        id_ = random.randint(0, 10*len(data))
        name = random_string(20)
        try:
            cur.execute('update user set name = "%s" where id > %s' %
                        (name, id_))
            cur.execute('select * from user where id > %s' % id_)
            res = cur.fetchall()
            res = [i for i in res]
            expect_res = [(i[0], i[1], name) for i in data if i[0] > id_]
            assert res == expect_res, 'the result of query is not expected %s' % line()
        except Exception as e:
            print('error: ', e)

    cur.execute('select * from user')
    data = cur.fetchall()
    data = [i for i in data]
    data.sort()

    print('update by age')

   # update age
    for _ in range(test_cnt_each_case):
        id_ = random.randint(0, 10*len(data))
        age = random.randint(0, 99)
        try:
            cur.execute('update user set age = %s where id > %s' %
                        (age, id_))
            cur.execute('select * from user where id > %s' % id_)
            res = cur.fetchall()
            res = [i for i in res]
            expect_res = [(i[0], age, i[2]) for i in data if i[0] > id_]

            assert res == expect_res, 'the result of query is not expected %s' % line()
        except Exception as e:
            print('error: ', e)

    cur.execute('select * from user')
    data = cur.fetchall()
    data = [i for i in data]

    # update name by age
    for _ in range(test_cnt_each_case):
        age = random.randint(0, 99)
        name = random_string(20)
        try:
            cur.execute('update user set name = "%s" where age < %s' %
                        (name, age))
            cur.execute('select * from user where age < %s' % age)
            res = cur.fetchall()
            res = [i for i in res]
            expect_res = [(i[0], i[1], name) for i in data if i[1] < age]
            res.sort()
            expect_res.sort()
            assert res == expect_res, 'the result of query is not expected %s' % line()
        except Exception as e:
            print('error: ', e)
    cur.close()


@log
def delete(con: pymysql.Connection, data: list):
    cur = con.cursor()
    cur.execute('select * from user')
    data = cur.fetchall()
    data = [i for i in data]
    # delete by age
    age = 80
    cur.execute('delete from user where age > %s ' % age)
    cur.execute('select * from user')
    res = cur.fetchall()
    res = [i for i in res]
    data.sort()
    data = [i for i in data if i[1] <= age]
    assert data == res, 'the result of query is not expected %s' % line()

    # delete by id
    for _ in range(5):
        id_ = random.randint(0, 10*len(data))
        cur.execute('delete from user where id < %s ' % id_)
        cur.execute('select * from user')
        res = cur.fetchall()
        res = [i for i in res]
        data = [i for i in data if i[0] >= id_]
        assert data == res, 'the result of query is not expected %s' % line()


if __name__ == "__main__":
    engine = 'InnoDB'
    con = connect()
    data_len = 20000
    create(con)
    data = generate_data(data_len)
    insert(con, data)
    query(con, data)
    condition_query(con, data)
    update(con, data)
    delete(con, data)
    con.close()
