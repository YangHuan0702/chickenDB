//
// Created by 杨欢 on 2026/5/1.
//
#include "gtest/gtest.h"
#include "parser/expression/binary_op_expression.h"
#include "parser/expression/column_expression.h"
#include "parser/expression/constant_expression.h"
#include "parser/statment/select_sql_statement.h"
#include "parser_test_util.h"

using namespace chickenDB;

TEST(Parser, SelectStarStatementTest) {
    std::string sql = "select * from user;";
    std::unique_ptr<SelectStatement> statement(
        parser_test::TransformSingleStatement<SelectStatement>(sql));

    ASSERT_NE(statement, nullptr);
    EXPECT_EQ(statement->type_, StatementType::SELECT);
    EXPECT_EQ(statement->table_, "user");
    ASSERT_EQ(statement->columns_.size(), 1U);
    EXPECT_EQ(statement->columns_[0], nullptr);
    EXPECT_EQ(statement->where_, nullptr);
}

TEST(Parser, SelectColumnsWhereGroupHavingOrderTest) {
    std::string sql =
        "select id, name from user where age > 18 group by id having id > 0 order by name;";
    std::unique_ptr<SelectStatement> statement(
        parser_test::TransformSingleStatement<SelectStatement>(sql));

    ASSERT_NE(statement, nullptr);
    EXPECT_EQ(statement->table_, "user");
    ASSERT_EQ(statement->columns_.size(), 2U);

    auto *id_column = dynamic_cast<ColumnRefExpression *>(statement->columns_[0].get());
    ASSERT_NE(id_column, nullptr);
    EXPECT_EQ(id_column->column_name_, "id");

    auto *name_column = dynamic_cast<ColumnRefExpression *>(statement->columns_[1].get());
    ASSERT_NE(name_column, nullptr);
    EXPECT_EQ(name_column->column_name_, "name");

    auto *where = dynamic_cast<BinaryOpExpression *>(statement->where_.get());
    ASSERT_NE(where, nullptr);
    EXPECT_EQ(where->type_, BinaryOpExpressionType::GT);
    auto *where_right = dynamic_cast<ConstantExpression *>(where->right_.get());
    ASSERT_NE(where_right, nullptr);
    EXPECT_EQ(parser_test::GetValue<int64_t>(where_right->val_), 18);

    ASSERT_EQ(statement->group_.size(), 1U);
    auto *group_column = dynamic_cast<ColumnRefExpression *>(statement->group_[0].get());
    ASSERT_NE(group_column, nullptr);
    EXPECT_EQ(group_column->column_name_, "id");

    auto *having = dynamic_cast<BinaryOpExpression *>(statement->having_.get());
    ASSERT_NE(having, nullptr);
    EXPECT_EQ(having->type_, BinaryOpExpressionType::GT);

    ASSERT_EQ(statement->order_.size(), 1U);
    auto *order_column = dynamic_cast<ColumnRefExpression *>(statement->order_[0].get());
    ASSERT_NE(order_column, nullptr);
    EXPECT_EQ(order_column->column_name_, "name");
}
