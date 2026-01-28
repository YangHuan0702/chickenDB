//
// Created by huan.yang on 2026-01-27.
//
#include "gtest/gtest.h"
#include "parser/parser.h"



namespace chickenDB {

    TEST(PARSER_TEST, SQL_TEST) {
        std::string sql = "select * from user";

        Parser parser;
        parser.ParserQuery(sql);

    }

}
