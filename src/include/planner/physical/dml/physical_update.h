//
// Created by huan.yang on 2026-06-11.
//
#pragma once
#include <memory>
#include <vector>

#include "common/types.h"
#include "common/value.h"
#include "binder/expression/bound_expression.h"
#include "planner/physical/physical_operator.h"

namespace chickenDB {
    // 物理 UPDATE。MVCC 语义：对命中行执行“删旧行 + 插新行”。新行 = 旧行各列值，
    // 但 col_ids_ 指定的列替换为 values_。执行由 Execution::ExecuteUpdate 分派。
    class PhysicalUpdate : public PhysicalOperator {
    public:
        explicit PhysicalUpdate(table_id_t table_id, std::vector<col_id_t> col_ids,
                                std::vector<Value> values, std::unique_ptr<BoundExpression> predicate)
            : PhysicalOperator(PhysicalOperatorType::UPDATE),
              table_id_(table_id), col_ids_(std::move(col_ids)),
              values_(std::move(values)), predicate_(std::move(predicate)) {}
        ~PhysicalUpdate() override = default;

        auto Init() -> void override;
        auto Next() -> Chunk * override;
        auto Close() -> void override;

        table_id_t table_id_;
        std::vector<col_id_t> col_ids_;          // 要更新的列
        std::vector<Value> values_;              // 对应的新值
        std::unique_ptr<BoundExpression> predicate_;
    };
}
