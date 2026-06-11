//
// Created by huan.yang on 2026-05-09.
//
#pragma once
#include <vector>

#include "physical_operator.h"
#include "common/types.h"

namespace chickenDB {
    class PhysicalProject : public PhysicalOperator {
    public:
        explicit PhysicalProject() : PhysicalOperator(PhysicalOperatorType::Project) {
        }

        auto Init() -> void override;

        auto Next() -> Chunk * override;

        auto Close() -> void override;

        ~PhysicalProject() override = default;

        std::vector<col_id_t> cols_;

    private:
        Chunk output_;
        std::vector<size_t> src_idx_; // cols_ 对应在输入 chunk 中的列下标（首批解析）
    };
}
