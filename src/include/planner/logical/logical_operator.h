//
// Created by huan.yang on 2026-05-09.
//
#pragma once
#include <memory>
#include <vector>

#include "common/operator_type.h"

namespace chickenDB {

    class LogicalOperator {
    public:
        explicit LogicalOperator(LogicalOperatorType type) : type_(type) {}
        virtual ~LogicalOperator() = default;

        LogicalOperatorType type_;
        std::vector<std::unique_ptr<LogicalOperator>> children_;
    };

}
