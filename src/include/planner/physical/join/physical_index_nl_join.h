//
// Created by huan.yang on 2026-05-09.
//
#pragma once
#include <vector>

#include "planner/physical/physical_operator.h"
#include "common/types.h"

namespace chickenDB {
    // 索引嵌套循环连接：build 侧建哈希索引，probe 侧每行用 join key 点查匹配（O(1) 探测）。
    // 等值连接，输出左右列拼接。
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
