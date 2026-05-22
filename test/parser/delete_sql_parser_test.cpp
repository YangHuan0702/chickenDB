//
// Created by 杨欢 on 2026/5/1.
//
#include "gtest/gtest.h"
#include "parser/expression/binary_op_expression.h"
#include "parser/expression/column_expression.h"
#include "parser/expression/constant_expression.h"
#include "parser/statment/delete_sql_statement.h"
#include "parser_test_util.h"

using namespace chickenDB;

TEST(Parser, DeleteStatementTest) {
    std::string sql = "delete from user where id = 1;";
    std::unique_ptr<DeleteStatement> statement(
        parser_test::TransformSingleStatement<DeleteStatement>(sql));

    ASSERT_NE(statement, nullptr);
    EXPECT_EQ(statement->type_, StatementType::DELETE);
    EXPECT_EQ(statement->table_name_, "user");
    ASSERT_NE(statement->where_, nullptr);
    EXPECT_EQ(statement->where_->type_, ParserExpressionType::BINARY_OP);

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

TEST(Parser, DeleteAndPredicateTest) {
    std::string sql = "delete from user where age >= 18 and name != 'Tom';";
    std::unique_ptr<DeleteStatement> statement(
        parser_test::TransformSingleStatement<DeleteStatement>(sql));

    ASSERT_NE(statement, nullptr);
    auto *where = dynamic_cast<BinaryOpExpression *>(statement->where_.get());
    ASSERT_NE(where, nullptr);
    EXPECT_EQ(where->type_, BinaryOpExpressionType::AND);

    auto *left = dynamic_cast<BinaryOpExpression *>(where->left_.get());
    ASSERT_NE(left, nullptr);
    EXPECT_EQ(left->type_, BinaryOpExpressionType::GTE);

    auto *right = dynamic_cast<BinaryOpExpression *>(where->right_.get());
    ASSERT_NE(right, nullptr);
    EXPECT_EQ(right->type_, BinaryOpExpressionType::NE);
}
