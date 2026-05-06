//
// Created by huan.yang on 2026-04-30.
//
#pragma once
#include <cstdint>

#include "sql/ColumnType.h"

namespace chickenDB {

    enum class StatementType : uint8_t {
        INSERT = 1,
        SELECT = 2,
        UPDATE = 3,
        DELETE = 4,

        CREATE = 5,
        DROP = 6,
    };


    enum class ColumnType : uint8_t {
        NUMBER,
        VARCHAR,
        VARCHAR2,
        DOUBLE,
    };


    static auto ConversionChickenDBColumnType(hsql::ColumnType column_type) -> ColumnType;


}
