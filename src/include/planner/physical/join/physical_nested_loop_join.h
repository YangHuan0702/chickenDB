//
// Created by huan.yang on 2026-05-09.
//
#pragma once
#include <vector>

#include "planner/physical/physical_operator.h"
#include "common/types.h"

namespace chickenDB {
    // 嵌套循环等值连接：Child(0)=left, Child(1)=right。
    // Init 物化右表全部行；Next 对左表每行扫描右表匹配（left_keys_[i]==right_keys_[i]），
    // 命中则输出 左列拼右列。最通用、无前置假设。
    class PhysicalNestedLoopJoin : public PhysicalOperator {
    public:
        explicit PhysicalNestedLoopJoin(std::vector<col_id_t> left_keys, std::vector<col_id_t> right_keys)
            : PhysicalOperator(PhysicalOperatorType::NestedLoopJoin),
              left_keys_(std::move(left_keys)), right_keys_(std::move(right_keys)) {}
        ~PhysicalNestedLoopJoin() override = default;

        auto Init() -> void override;
        auto Next() -> Chunk * override;
        auto Close() -> void override;

        std::vector<col_id_t> left_keys_;
        std::vector<col_id_t> right_keys_;

    private:
        // 右表整体物化：行值(double) + 类型/col_ids。
        std::vector<std::vector<double>> right_rows_;
        std::vector<ColumnType> right_types_;
        std::vector<col_id_t> right_ids_;
        std::vector<size_t> right_key_idx_;
        bool right_built_{false};
        Chunk output_;
    };
}
