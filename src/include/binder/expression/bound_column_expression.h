//
// Created by huan.yang on 2026-05-08.
//
#pragma once
#include "bound_expression.h"
#include "common/types.h"

namespace chickenDB {
    class BoundColumnExpression : public BoundExpression {
    public:
        explicit BoundColumnExpression(table_id_t table_id, col_id_t col_id) : BoundExpression(
                                                                                   BinderExpressionType::COLUMN),
                                                                               table_id_(table_id), col_id_(col_id) {
        }

        ~BoundColumnExpression() override = default;

        table_id_t table_id_;
        col_id_t col_id_;
    };
}
