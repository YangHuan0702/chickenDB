//
// Created by 杨欢 on 2026/5/6.
//
#pragma once

namespace chickenDB {

    enum class BinaryOpExpressionType {
        AND,
        OR,
        GT,
        GTE,
        LT,
        LTE,
        ADD,
        SUB,
        MUL,
        DRI,
        EQ,
        NE,
        LIKE,
        NOT_LIKE,
        ILIKE,
        IN
    };

}