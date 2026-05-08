//
// Created by huan.yang on 2026-05-08.
//
#pragma once

namespace chickenDB {
    enum class BinderExpressionType {
        CONSTANT,
        COLUMN,
        BINARY_OP,
        UNARY_OP,
        IN,
    };
}
