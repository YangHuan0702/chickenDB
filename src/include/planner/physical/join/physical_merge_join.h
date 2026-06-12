//
// Created by huan.yang on 2026-05-09.
//
#pragma once
#include <vector>

#include "planner/physical/physical_operator.h"
#include "common/types.h"

namespace chickenDB {
    // 归并等值连接：前提两侧都按 join key 有序。Init 物化两侧并各自按 key 排序，
    // Next 用双指针归并匹配，一次性产出全部匹配行。
    class PhysicalMergeJoin : public PhysicalOperator {
    public:
        explicit PhysicalMergeJoin(std::vector<col_id_t> left_keys, std::vector<col_id_t> right_keys)
            : PhysicalOperator(PhysicalOperatorType::MergeJoin),
              left_keys_(std::move(left_keys)), right_keys_(std::move(right_keys)) {}
        ~PhysicalMergeJoin() override = default;

        auto Init() -> void override;
        auto Next() -> Chunk * override;
        auto Close() -> void override;

        std::vector<col_id_t> left_keys_;
        std::vector<col_id_t> right_keys_;

    private:
        bool built_{false};
        Chunk output_;
    };
}
