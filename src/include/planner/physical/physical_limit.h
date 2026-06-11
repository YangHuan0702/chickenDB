//
// Created by huan.yang on 2026-05-09.
//
#pragma once
#include <vector>

#include "physical_operator.h"

namespace chickenDB {
    class PhysicalLimit : public PhysicalOperator {
    public:
        explicit PhysicalLimit(size_t start, size_t offset) : PhysicalOperator(PhysicalOperatorType::Limit),
                                                              start_(start), offset_(offset) {
        }

        auto Init() -> void override;

        auto Next() -> Chunk * override;

        auto Close() -> void override;

        ~PhysicalLimit() override = default;

        size_t start_;
        size_t offset_;

    private:
        Chunk output_;
        size_t seen_{0};      // 已从孩子读到的行数（用于跳过 start_）
        size_t emitted_{0};   // 已输出的行数（上限 offset_）
    };
}
