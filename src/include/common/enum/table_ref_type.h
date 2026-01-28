//
// Created by huan.yang on 2026-01-27.
//
#pragma once
#include <cstdint>

namespace chickenDB {

    enum class TableReferenceType: uint8_t {
        INVALID = 0,
        BASE_TABLE = 1,
        SUBQUERY = 2,
        JOIN = 3,
        CROSS_PRODUCT = 4,
        TABLE_FUNCTION = 5
    };

}
