//
// Created by huan.yang on 2026-01-28.
//

#pragma once
#include "parser/parsed_expression.h"
#include "parser/table_ref.h"

namespace chickenDB {
    class TableFunction : public TableRef {
    public:
        TableFunction() : TableRef(TableReferenceType::TABLE_FUNCTION) {}

        std::unique_ptr<ParsedExpression> function;

        std::string ToString() const override {
            return function->ToString();
        }

        bool Equals(const TableRef *other) override;

        std::unique_ptr<TableRef> Copy() const override;
    };
}
