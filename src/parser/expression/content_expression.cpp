//
// Created by huan.yang on 2026-01-28.
//
#include "parser/expression/content_expression.h"

namespace chickenDB {

    ContentExpression::ContentExpression(SQLType sql_type, Value val) : ParsedExpression(ExpressionType::VALUE_CONSTANT,ExpressionClass::CONSTANT),
    val(val),sql_type(sql_type){
    }

    std::unique_ptr<ParsedExpression> ContentExpression::Copy() const {
        auto copy = std::make_unique<ContentExpression>(sql_type, val);
        copy->CopyProperties(*this);
        return std::move(copy);
    }


    uint64_t ContentExpression::Hash() const {
        return 0;
    }

    std::string ContentExpression::ToString() const {
        return val.str_value;
    }

    bool ContentExpression::Equals(ColumnRefExpression *other_) const {
        return false;
    }


}