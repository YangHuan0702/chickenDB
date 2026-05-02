//
// Created by 杨欢 on 2025/1/21.
//

#include <gtest/gtest.h>

#include "parser/parser.h"

using namespace chickenDB;

TEST(ParserSQLTest,printParseredSQL) {
    Parser parser;
    std::string sql = "select * from user";
    parser.ParserQuery(sql);

}