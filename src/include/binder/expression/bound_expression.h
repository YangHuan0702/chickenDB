//
// Created by huan.yang on 2026-05-08.
//
#pragma once
#include "common/enum/binder_expression_type.h"

namespace chickenDB {
    class BoundExpression {
    public:
        explicit BoundExpression(BinderExpressionType type) : type_(type) {
        }

        virtual ~BoundExpression() = default;

        BinderExpressionType type_;
    };
}
