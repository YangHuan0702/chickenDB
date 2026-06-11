//
// Created by huan.yang on 2026-05-09.
//
#pragma once
#include <vector>

#include "planner/physical/physical_operator.h"
#include "common/types.h"

namespace chickenDB {
    // 内存排序：Init 阶段把孩子的全部 chunk 物化成行集合，按排序键排序；
    // Next 分批吐出。排序键为 sort_cols_（按 col_id），均按升序（v1）。
    class PhysicalInMemorySort : public PhysicalOperator {
    public:
        explicit PhysicalInMemorySort(std::vector<col_id_t> sort_cols)
            : PhysicalOperator(PhysicalOperatorType::InMemorySort), sort_cols_(std::move(sort_cols)) {}
        ~PhysicalInMemorySort() override = default;

        auto Init() -> void override;
        auto Next() -> Chunk * override;
        auto Close() -> void override;

        std::vector<col_id_t> sort_cols_;

    private:
        // 物化的所有行（按 sorted 后顺序），逐行存为列值的 double（定长）。
        std::vector<std::vector<double>> rows_;
        std::vector<ColumnType> types_;
        std::vector<col_id_t> col_ids_;
        Chunk output_;
        size_t emit_pos_{0};
        bool built_{false};
    };
}
