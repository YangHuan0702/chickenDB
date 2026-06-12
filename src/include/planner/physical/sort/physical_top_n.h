//
// Created by huan.yang on 2026-05-09.
//
#pragma once
#include <vector>

#include "planner/physical/physical_operator.h"
#include "common/types.h"

namespace chickenDB {
    // TopN：排序后只保留前 n_ 行。用 std::partial_sort（O(N log n)）取前 n。
    class PhysicalTopN : public PhysicalOperator {
    public:
        explicit PhysicalTopN(std::vector<col_id_t> sort_cols, size_t n)
            : PhysicalOperator(PhysicalOperatorType::TopN), sort_cols_(std::move(sort_cols)), n_(n) {}
        ~PhysicalTopN() override = default;

        auto Init() -> void override;
        auto Next() -> Chunk * override;
        auto Close() -> void override;

        std::vector<col_id_t> sort_cols_;
        size_t n_;

    private:
        std::vector<std::vector<double>> rows_;
        std::vector<ColumnType> types_;
        std::vector<col_id_t> col_ids_;
        Chunk output_;
        bool built_{false};
    };
}
