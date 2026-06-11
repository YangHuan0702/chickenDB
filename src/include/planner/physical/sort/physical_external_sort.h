//
// Created by huan.yang on 2026-05-09.
//
#pragma once
#include <vector>

#include "planner/physical/physical_operator.h"
#include "common/types.h"

namespace chickenDB {
    // 外部排序：内存不足时 spill 到磁盘归并。v1 退化为内存排序（spill 留 TODO），
    // 与 InMemorySort 逻辑一致，仅类型标记不同，便于 planner/优化器后续区分。
    class PhysicalExternalSort : public PhysicalOperator {
    public:
        explicit PhysicalExternalSort(std::vector<col_id_t> sort_cols)
            : PhysicalOperator(PhysicalOperatorType::ExternalSort), sort_cols_(std::move(sort_cols)) {}
        ~PhysicalExternalSort() override = default;

        auto Init() -> void override;
        auto Next() -> Chunk * override;
        auto Close() -> void override;

        std::vector<col_id_t> sort_cols_;

    private:
        std::vector<std::vector<double>> rows_;
        std::vector<ColumnType> types_;
        std::vector<col_id_t> col_ids_;
        Chunk output_;
        size_t emit_pos_{0};
        bool built_{false};
    };
}
