//
// Created by huan.yang on 2026-05-08.
//
#pragma once
#include <memory>
#include <vector>

#include "bound_statement.h"
#include "binder/expression/bound_expression.h"
#include "common/types.h"
#include "common/value.h"

namespace chickenDB {
    class BoundUpdateStatement : public BoundStatement {
    public:
        explicit BoundUpdateStatement(table_id_t table_id) : BoundStatement(StatementType::UPDATE),
                                                             table_id_(table_id) {
        }
        ~BoundUpdateStatement() override = default;

        table_id_t table_id_;
        std::vector<col_id_t> col_ids_;
        std::vector<Value> values_;
        std::unique_ptr<BoundExpression> where_;
    };
}
