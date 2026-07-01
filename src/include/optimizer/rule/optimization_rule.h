//
// Created by huan.yang on 2026-06-30.
//
#pragma once
#include <memory>
#include <string>

#include "planner/logical/logical_operator.h"

namespace chickenDB {

    // 优化规则抽象基类。
    // 每条规则接受一棵逻辑计划树，对其做等价变换，返回新树。
    // 规则不保证每次都产生变化；若无适用场景，原样返回即可。
    class OptimizationRule {
    public:
        virtual ~OptimizationRule() = default;

        // 对 plan 应用本规则，返回（可能已变换的）等价计划。
        virtual auto Apply(std::unique_ptr<LogicalOperator> plan)
            -> std::unique_ptr<LogicalOperator> = 0;

        virtual auto Name() const -> std::string = 0;
    };

}
