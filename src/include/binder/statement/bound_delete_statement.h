//
// Created by huan.yang on 2026-05-08.
//
#pragma once
#include <memory>

#include "bound_statement.h"
#include "binder/expression/bound_expression.h"
#include "common/types.h"

namespace chickenDB {
    class BoundDeleteStatement : public BoundStatement {
    public:
        explicit BoundDeleteStatement(table_id_t table_id) : BoundStatement(StatementType::DELETE),table_id_(table_id) {
        }
        ~BoundDeleteStatement() override = default;


        table_id_t table_id_;
        std::unique_ptr<BoundExpression> where_;
    };
}
