//
// Created by 杨欢 on 2026/5/1.
//
#include "gtest/gtest.h"
#include "parser/expression/binary_op_expression.h"
#include "parser/expression/column_expression.h"
#include "parser/expression/constant_expression.h"
#include "parser/statment/update_sql_statement.h"
#include "parser_test_util.h"

using namespace chickenDB;

TEST(Parser, UpdateStatementTest) {
    std::string sql = "update user set name = 'Bob', age = 20 where id = 1;";
    std::unique_ptr<UpdateStatement> statement(
        parser_test::TransformSingleStatement<UpdateStatement>(sql));

    ASSERT_NE(statement, nullptr);
    EXPECT_EQ(statement->type_, StatementType::UPDATE);
    EXPECT_EQ(statement->table_name_, "user");
    ASSERT_EQ(statement->columns_.size(), 2U);
    ASSERT_EQ(statement->values_.size(), 2U);

    EXPECT_EQ(statement->columns_[0], "name");
    EXPECT_EQ(parser_test::GetValue<std::string>(statement->values_[0]), "Bob");

    EXPECT_EQ(statement->columns_[1], "age");
    EXPECT_EQ(parser_test::GetValue<int64_t>(statement->values_[1]), 20);

    auto *where = dynamic_cast<BinaryOpExpression *>(statement->where_.get());
    ASSERT_NE(where, nullptr);
    EXPECT_EQ(where->type_, BinaryOpExpressionType::EQ);

    auto *left = dynamic_cast<ColumnRefExpression *>(where->left_.get());
    ASSERT_NE(left, nullptr);
    EXPECT_EQ(left->column_name_, "id");

    auto *right = dynamic_cast<ConstantExpression *>(where->right_.get());
    ASSERT_NE(right, nullptr);
    EXPECT_EQ(parser_test::GetValue<int64_t>(right->val_), 1);
}

TEST(Parser, UpdateDoubleValueTest) {
    std::string sql = "update orders set price = 99.9 where id = 10;";
    std::unique_ptr<UpdateStatement> statement(
        parser_test::TransformSingleStatement<UpdateStatement>(sql));

    ASSERT_NE(statement, nullptr);
    ASSERT_EQ(statement->columns_.size(), 1U);
    ASSERT_EQ(statement->values_.size(), 1U);
    EXPECT_EQ(statement->columns_[0], "price");
    EXPECT_DOUBLE_EQ(parser_test::GetValue<double>(statement->values_[0]), 99.9);
}
