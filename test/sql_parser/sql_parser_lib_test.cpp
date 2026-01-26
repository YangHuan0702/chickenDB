//
// Created by 杨欢 on 2025/1/22.
//
#include "gtest/gtest.h"
#include "SQLParser.h"

TEST(LIB_SQLPARSER,stringSQLParser) {
    const std::string sql = "select * from user";
    hsql::SQLParserResult result;
    hsql::SQLParser::parse(sql,&result);

    std::cout << "lib sql-parser included" << std::endl;
}