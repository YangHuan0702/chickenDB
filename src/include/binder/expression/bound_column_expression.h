//
// Created by huan.yang on 2026-05-08.
//
#pragma once
#include <string>

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
        bool is_aggregate_{false}; // 该列引用是否来自聚合函数（如 SUM(col)）
        std::string agg_func_;     // 聚合函数名（SUM/COUNT/MIN/MAX/AVG），非聚合为空
    };
}
