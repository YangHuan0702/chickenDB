//
// Created by huan.yang on 2026-05-09.
//
#pragma once
#include <vector>

#include "planner/physical/physical_operator.h"
#include "common/types.h"

namespace chickenDB {
    // 索引嵌套循环连接：本应对 probe 侧每行用 build 侧索引点查。无索引时 v1 退化为
    // 嵌套循环语义（全表匹配）。语义与 NestedLoopJoin 一致，仅类型标记不同。
    class PhysicalIndexNLJoin : public PhysicalOperator {
    public:
        explicit PhysicalIndexNLJoin(std::vector<col_id_t> left_keys, std::vector<col_id_t> right_keys)
            : PhysicalOperator(PhysicalOperatorType::IndexNLJoin),
              left_keys_(std::move(left_keys)), right_keys_(std::move(right_keys)) {}
        ~PhysicalIndexNLJoin() override = default;

        auto Init() -> void override;
        auto Next() -> Chunk * override;
        auto Close() -> void override;

        std::vector<col_id_t> left_keys_;
        std::vector<col_id_t> right_keys_;

    private:
        std::vector<std::vector<double>> right_rows_;
        std::vector<ColumnType> right_types_;
        std::vector<col_id_t> right_ids_;
        std::vector<size_t> right_key_idx_;
        bool built_{false};
        Chunk output_;
    };
}
