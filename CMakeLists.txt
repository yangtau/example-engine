cmake_minimum_required(VERSION 2.8)
project(example_engine)

# set(include_dir 
#     "C/Users/qyang/code/mysql-5.7.10/sql"
#     "C/Users/qyang/code/mysql-5.7.10/include/mysql" 
#     "C/Users/qyang/code/mysql-5.7.10/include"
#     "./lib"
#     )
# include_directories(include_dir)

include_directories("C/Users/qyang/code/mysql-5.7.10/sql")

aux_source_directory(. DIR_SRCS)

add_subdirectory(lib)

add_library(ha_example STATIC ${DIR_SRCS})

target_link_libraries(ha_example BTree)