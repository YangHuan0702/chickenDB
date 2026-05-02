//
// Created by 杨欢 on 2026/5/1.
//
#include "parser/parser.h"
#include "gtest/gtest.h"

using namespace std;
using namespace chickenDB;

TEST(Parser,CreateTableStatementTest) {
    std::string sql = "create table user (id varchar(30),name varchar(30),age integer);";
    Parser parser;
    parser.ParserQuery(sql);
}
