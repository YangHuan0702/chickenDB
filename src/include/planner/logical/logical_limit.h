//
// Created by huan.yang on 2026-05-09.
//
#pragma once
#include <cstdint>

#include "logical_operator.h"

namespace chickenDB {
    class LogicalLimit : public LogicalOperator {
    public:
        explicit LogicalLimit(int32_t start, int32_t size) : LogicalOperator(LogicalOperatorType::LIMIT), start_(start),
                                                             size_(size) {
        }

        ~LogicalLimit() override = default;


        int32_t start_;
        int32_t size_;
    };
}
