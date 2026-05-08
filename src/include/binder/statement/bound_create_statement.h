//
// Created by huan.yang on 2026-05-08.
//
#pragma once
#include <vector>

#include "bound_statement.h"
#include "common/types.h"

namespace chickenDB {
    class BoundCreateStatement : public BoundStatement {
    public:
        explicit BoundCreateStatement(table_id_t table_id) : BoundStatement(StatementType::CREATE),
                                                             table_id_(table_id) {
        }

        ~BoundCreateStatement() override = default;

        auto AddColumn(col_id_t col_id) -> void {
            this->col_ids_.push_back(col_id);
        }

        table_id_t table_id_;
        std::vector<col_id_t> col_ids_;
    };
}
