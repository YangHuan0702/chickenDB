//
// Created by huan.yang on 2026-05-09.
//
#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "planner/physical/physical_operator.h"
#include "common/types.h"

namespace chickenDB {
    // 哈希等值连接：Child(0)=build/left, Child(1)=probe/right。
    // Init 对 build 侧建哈希表（join key -> 行列表）；Next 对 probe 侧每行查表拼接。
    class PhysicalHashJoin : public PhysicalOperator {
    public:
        explicit PhysicalHashJoin(std::vector<col_id_t> left_keys, std::vector<col_id_t> right_keys)
            : PhysicalOperator(PhysicalOperatorType::HashJoin),
              left_keys_(std::move(left_keys)), right_keys_(std::move(right_keys)) {}
        ~PhysicalHashJoin() override = default;

        auto Init() -> void override;
        auto Next() -> Chunk * override;
        auto Close() -> void override;

        std::vector<col_id_t> left_keys_;
        std::vector<col_id_t> right_keys_;

    private:
        // build 侧物化 + 哈希表：key 字节串 -> build 行下标列表。
        std::vector<std::vector<double>> build_rows_;
        std::vector<ColumnType> build_types_;
        std::vector<col_id_t> build_ids_;
        std::vector<size_t> build_key_idx_;
        std::unordered_map<std::string, std::vector<size_t>> hash_table_;
        bool built_{false};
        Chunk output_;
    };
}
