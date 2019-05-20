#include "Source.cpp"

int main() {

    initial();
    vector<RowData> data = {
        {12, 0, "Hello", "World", "hahha" },
        {123, 1, "Hello", "World", "hahha" },
        {2, 0, "Hello", "World", "hahha" },
        {1200, 1, "Hello", "World", "hahha" },

        {12, 0, "Hi", "World", "hahha" },
        {1273, 1, "jay", "World", "hahha" },
        {20, 0, "chou", "World", "hahha" },
        {120, 1, "ahah", "World", "hahha" },
    };

    insert(data);

    auto res = query("select * from table where id >= 0");
    std::cout << "----------\n";
    for (auto &i : res) showRowData(i);

    del("delete from table where id = 2");

    res = query("select * from table where id >= 0");
    std::cout << "----------\n";
    for (auto &i : res) showRowData(i);

    update("update table set number = 20157894 where id<900");

    res = query("select * from table where id >= 0");
    std::cout << "----------\n";
    for (auto &i : res) showRowData(i);



    getchar();

    return 0;
}