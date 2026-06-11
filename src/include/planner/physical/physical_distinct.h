//
// Created by huan.yang on 2026-05-09.
//

#pragma once
#include <string>
#include <unordered_set>
#include <vector>

#include "physical_operator.h"
#include "common/types.h"

namespace chickenDB {
    class PhysicalDistinct : public PhysicalOperator {
    public:
        explicit PhysicalDistinct() : PhysicalOperator(PhysicalOperatorType::Distinct) {
        }

        auto Init() -> void override;

        auto Next() -> Chunk * override;

        auto Close() -> void override;

        ~PhysicalDistinct() override = default;

        std::vector<col_id_t> cols_;

    private:
        Chunk output_;
        std::unordered_set<std::string> seen_; // 已出现的行键（各列字节拼接）
    };
}
