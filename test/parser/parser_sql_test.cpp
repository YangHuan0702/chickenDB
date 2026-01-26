//
// Created by 杨欢 on 2025/1/21.
//

#include <gtest/gtest.h>
#include <parser/postgres_parser.h>

#include "parser/parser.h"

TEST(ParserSQLTest,printParseredSQL) {
    haloTP::PostgresParser parser;
    std::string sql = "select * from user";
    parser.Parser(sql);

}