//
// Created by huan.yang on 2026-06-11.
//
#pragma once
#include <memory>

#include "common/types.h"
#include "binder/expression/bound_expression.h"
#include "planner/physical/physical_operator.h"

namespace chickenDB {
    // 物理 DELETE。执行由 Execution::ExecuteDelete 分派：扫表按 predicate_ 匹配行，
    // 对命中行 RID 做 MVCC 删除（version store OnDelete + WAL）。
    class PhysicalDelete : public PhysicalOperator {
    public:
        explicit PhysicalDelete(table_id_t table_id, std::unique_ptr<BoundExpression> predicate)
            : PhysicalOperator(PhysicalOperatorType::DELETE),
              table_id_(table_id), predicate_(std::move(predicate)) {}
        ~PhysicalDelete() override = default;

        auto Init() -> void override;
        auto Next() -> Chunk * override;
        auto Close() -> void override;

        table_id_t table_id_;
        std::unique_ptr<BoundExpression> predicate_; // 可空 = 删全表
    };
}
