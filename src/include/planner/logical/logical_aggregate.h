//
// Created by huan.yang on 2026-05-09.
//
#pragma once
#include "logical_operator.h"

namespace chickenDB {

    class LogicalAggregate : public LogicalOperator {
    public:
        explicit LogicalAggregate( ) : LogicalOperator(LogicalOperatorType::AGGREGATE) {}
        ~LogicalAggregate() override = default;


    };

}
