//
// Created by huan.yang on 2026-05-08.
//
#pragma once
#include <vector>

#include "bound_statement.h"
#include "common/types.h"
#include "common/value.h"

namespace chickenDB {
    class BoundInsertStatement : public BoundStatement {
    public:
        explicit BoundInsertStatement(table_id_t table_id) : BoundStatement(StatementType::INSERT),table_id_(table_id) {}
        ~BoundInsertStatement() override = default;

        table_id_t table_id_;
        std::vector<col_id_t> col_ids_;
        std::vector<Value> values_;
    };
}
