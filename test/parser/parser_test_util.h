#pragma once

#include <memory>
#include <string>
#include <vector>

#include "SQLParser.h"
#include "SQLParserResult.h"
#include "common/value.h"
#include "gtest/gtest.h"
#include "parser/transformer.h"

namespace chickenDB::parser_test {

inline auto TransformSql(const std::string &sql) -> std::vector<std::unique_ptr<SQLStatement>> {
    hsql::SQLParserResult result;
    hsql::SQLParser::parse(sql, &result);
    EXPECT_TRUE(result.isValid()) << result.errorMsg();

    std::vector<std::unique_ptr<SQLStatement>> statements;
    Transformer transformer;
    transformer.TransformerAST(result, statements);
    return statements;
}

template <typename Statement>
auto TransformSingleStatement(const std::string &sql) -> Statement * {
    auto statements = TransformSql(sql);
    EXPECT_EQ(statements.size(), 1U);
    auto *statement = dynamic_cast<Statement *>(statements.front().release());
    EXPECT_NE(statement, nullptr);
    return statement;
}

template <typename T>
auto GetValue(const Value &value) -> const T & {
    return std::get<T>(value.value_);
}

} // namespace chickenDB::parser_test
