//
// Created by huan.yang on 2026-05-09.
//
#pragma once
#include <vector>

#include "planner/physical/physical_operator.h"
#include "planner/physical/sort/sort_cell.h"
#include "common/types.h"

namespace chickenDB {
    // 内存排序：Init 阶段把孩子的全部 chunk 物化成行集合，按排序键排序；
    // Next 分批吐出。排序键 sort_cols_（按 col_id），sort_desc_ 并列指定每列升/降序。
    class PhysicalInMemorySort : public PhysicalOperator {
    public:
        explicit PhysicalInMemorySort(std::vector<col_id_t> sort_cols,
                                      std::vector<bool> sort_desc = {})
            : PhysicalOperator(PhysicalOperatorType::InMemorySort),
              sort_cols_(std::move(sort_cols)), sort_desc_(std::move(sort_desc)) {}
        ~PhysicalInMemorySort() override = default;

        auto Init() -> void override;
        auto Next() -> Chunk * override;
        auto Close() -> void override;

        std::vector<col_id_t> sort_cols_;
        std::vector<bool> sort_desc_;

    private:
        // 物化的所有行（按 sorted 后顺序）。每个单元格 SortCell：定长列存 num，变长列存 str。
        std::vector<std::vector<SortCell>> rows_;
        std::vector<ColumnType> types_;
        std::vector<col_id_t> col_ids_;
        Chunk output_;
        size_t emit_pos_{0};
        bool built_{false};
    };
}
