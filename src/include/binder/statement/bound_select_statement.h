//
// Created by huan.yang on 2026-05-08.
//
#pragma once
#include <memory>
#include <vector>

#include "bound_statement.h"
#include "binder/expression/bound_expression.h"
#include "common/types.h"

namespace chickenDB {
    class BoundSelectStatement : public BoundStatement {
    public:
        explicit BoundSelectStatement(table_id_t table_id) : BoundStatement(StatementType::SELECT),
                                                             table_id_(table_id) {
        }

        ~BoundSelectStatement() override = default;


        table_id_t table_id_;

        std::vector<std::unique_ptr<BoundExpression>> columns_;

        std::unique_ptr<BoundExpression> where_;

        std::vector<std::unique_ptr<BoundExpression> > group_;
        std::unique_ptr<BoundExpression> having_;

        std::vector<std::unique_ptr<BoundExpression> > order_;
        std::vector<bool> order_desc_; // 与 order_ 并列：true=降序

        // 两表 inner equi-join：join_table_id_ 为右表，join_condition_ 为 ON（已绑定）。
        bool has_join_{false};
        table_id_t join_table_id_{0};
        std::unique_ptr<BoundExpression> join_condition_;
    };
}
