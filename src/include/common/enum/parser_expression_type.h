//
// Created by 杨欢 on 2026/5/6.
//
#pragma once

namespace chickenDB {

    enum class ParserExpressionType {
        CONSTANT,
        COLUMN,
        BINARY_OP,
        UNARY_OP,
        IN,
    };

}
