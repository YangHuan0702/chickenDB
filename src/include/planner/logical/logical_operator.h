//
// Created by huan.yang on 2026-05-09.
//
#pragma once
#include "common/operator_type.h"

namespace chickenDB {

    class LogicalOperator {
    public:
        LogicalOperator(LogicalOperatorType type) : type_(type) {}
        virtual ~LogicalOperator() = default;


        LogicalOperatorType type_;
    };

}
