//
// Created by 杨欢 on 2026/5/1.
//
#include "gtest/gtest.h"
#include "parser/statment/create_table_statement.h"
#include "parser_test_util.h"

using namespace std;
using namespace chickenDB;

TEST(Parser, CreateTableStatementTest) {
    std::string sql = "create table user (id varchar(30),name varchar(30),age integer);";
    std::unique_ptr<CreateTableStatement> statement(
        parser_test::TransformSingleStatement<CreateTableStatement>(sql));

    ASSERT_NE(statement, nullptr);
    EXPECT_EQ(statement->type_, StatementType::CREATE);
    EXPECT_EQ(statement->table_name_, "user");
    ASSERT_EQ(statement->columns_.size(), 3U);

    EXPECT_EQ(statement->columns_[0].name_, "id");
    EXPECT_EQ(statement->columns_[0].type_, ColumnType::VARCHAR);
    EXPECT_EQ(statement->columns_[0].size_, 30U);

    EXPECT_EQ(statement->columns_[1].name_, "name");
    EXPECT_EQ(statement->columns_[1].type_, ColumnType::VARCHAR);
    EXPECT_EQ(statement->columns_[1].size_, 30U);

    EXPECT_EQ(statement->columns_[2].name_, "age");
    EXPECT_EQ(statement->columns_[2].type_, ColumnType::NUMBER);
    EXPECT_EQ(statement->columns_[2].size_, 0U);
}

TEST(Parser, CreateTableDoubleColumnTest) {
    std::string sql = "create table orders (id int, price double);";
    std::unique_ptr<CreateTableStatement> statement(
        parser_test::TransformSingleStatement<CreateTableStatement>(sql));

    ASSERT_NE(statement, nullptr);
    ASSERT_EQ(statement->columns_.size(), 2U);
    EXPECT_EQ(statement->columns_[1].name_, "price");
    EXPECT_EQ(statement->columns_[1].type_, ColumnType::DOUBLE);
}
