//
// Created by huan.yang on 2026-05-09.
//
#pragma once
#include <memory>
#include <unordered_map>

#include "physical_operator.h"
#include "binder/expression/bound_expression.h"
#include "common/types.h"

namespace chickenDB {
    class PhysicalFilter : public PhysicalOperator {
    public:
        explicit PhysicalFilter(std::unique_ptr<BoundExpression> expression) : PhysicalOperator(
                                                                                   PhysicalOperatorType::Filter),
                                                                               expression_(std::move(expression)) {
        }

        auto Init() -> void override;

        auto Next() -> Chunk * override;

        auto Close() -> void override;

        ~PhysicalFilter() override = default;

        std::unique_ptr<BoundExpression> expression_;

    private:
        Chunk output_;
        std::unordered_map<col_id_t, size_t> col_map_;
    };
}
