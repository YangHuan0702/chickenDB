//
// Created by huan.yang on 2026-05-09.
//
#pragma once
#include <memory>

#include "physical_operator.h"
#include "binder/expression/bound_expression.h"

namespace chickenDB {
    class PhysicalFilter : public PhysicalOperator {
    public:
        explicit PhysicalFilter(std::unique_ptr<BoundExpression> expression) : PhysicalOperator(
                                                                                   PhysicalOperatorType::Filter),
                                                                               expression_(std::move(expression)) {
        }

        ~PhysicalFilter() override = default;

        std::unique_ptr<BoundExpression> expression_;
    };
}
