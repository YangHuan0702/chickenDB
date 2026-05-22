//
// Created by 杨欢 on 2026/5/1.
//
#include "gtest/gtest.h"
#include "parser/statment/insert_sql_statement.h"
#include "parser_test_util.h"

using namespace chickenDB;

TEST(Parser, InsertStatementTest) {
    std::string sql = "insert into user (id, name, age) values (1, 'Alice', 18);";
    std::unique_ptr<InsertStatement> statement(
        parser_test::TransformSingleStatement<InsertStatement>(sql));

    ASSERT_NE(statement, nullptr);
    EXPECT_EQ(statement->type_, StatementType::INSERT);
    EXPECT_EQ(statement->table_name_, "user");
    ASSERT_EQ(statement->columns_.size(), 3U);
    ASSERT_EQ(statement->values_.size(), 3U);

    EXPECT_EQ(statement->columns_[0], "id");
    EXPECT_EQ(parser_test::GetValue<int64_t>(statement->values_[0]), 1);

    EXPECT_EQ(statement->columns_[1], "name");
    EXPECT_EQ(parser_test::GetValue<std::string>(statement->values_[1]), "Alice");

    EXPECT_EQ(statement->columns_[2], "age");
    EXPECT_EQ(parser_test::GetValue<int64_t>(statement->values_[2]), 18);
}

TEST(Parser, InsertFloatAndNullValuesTest) {
    std::string sql = "insert into orders (price, note) values (12.5, null);";
    std::unique_ptr<InsertStatement> statement(
        parser_test::TransformSingleStatement<InsertStatement>(sql));

    ASSERT_NE(statement, nullptr);
    ASSERT_EQ(statement->values_.size(), 2U);
    EXPECT_DOUBLE_EQ(parser_test::GetValue<double>(statement->values_[0]), 12.5);
    EXPECT_TRUE(std::holds_alternative<std::monostate>(statement->values_[1].value_));
}
