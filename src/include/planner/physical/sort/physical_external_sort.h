//
// Created by huan.yang on 2026-05-09.
//
#pragma once
#include <vector>

#include "planner/physical/physical_operator.h"
#include "common/types.h"

namespace chickenDB {
    // 外部排序：分批物化内存排序写 run 临时文件，再 K 路归并输出（spill-to-disk）。
    class PhysicalExternalSort : public PhysicalOperator {
    public:
        explicit PhysicalExternalSort(std::vector<col_id_t> sort_cols,
                                      std::vector<bool> sort_desc = {})
            : PhysicalOperator(PhysicalOperatorType::ExternalSort),
              sort_cols_(std::move(sort_cols)), sort_desc_(std::move(sort_desc)) {}
        ~PhysicalExternalSort() override = default;

        auto Init() -> void override;
        auto Next() -> Chunk * override;
        auto Close() -> void override;

        std::vector<col_id_t> sort_cols_;
        std::vector<bool> sort_desc_;

    private:
        std::vector<std::vector<double>> rows_;
        std::vector<ColumnType> types_;
        std::vector<col_id_t> col_ids_;
        Chunk output_;
        size_t emit_pos_{0};
        bool built_{false};
    };
}
