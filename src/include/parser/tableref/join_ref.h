//
// Created by huan.yang on 2026-01-27.
//

#pragma once
#include "parser/parsed_expression.h"
#include "parser/table_ref.h"
#include "common/enum/join_type.h"

#include <unordered_set>

namespace chickenDB {
    class JoinRef : public TableRef {
    public:
        JoinRef() : TableRef(TableReferenceType::JOIN) {}

        std::unique_ptr<TableRef> left;
        std::unique_ptr<TableRef> right;
        std::unique_ptr<ParsedExpression> condition;
        JoinType type;

        std::unordered_set<std::string> hidden_columns;

        bool Equals(const TableRef *other) override;
        std::unique_ptr<TableRef> Copy() const override;
    };
}
