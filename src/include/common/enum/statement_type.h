//
// Created by huan.yang on 2026-04-30.
//
#pragma once
#include <cstddef>
#include <cstdint>

namespace chickenDB {
    enum class StatementType : uint8_t {
        INSERT = 1,
        SELECT = 2,
        UPDATE = 3,
        DELETE = 4,

        CREATE = 5,
        DROP = 6,
        CREATE_INDEX = 7,
        TRANSACTION = 8,
    };


    enum class ColumnType : uint8_t {
        NUMBER,
        VARCHAR,
        VARCHAR2,
        DOUBLE,
    };

    class TypeSizeConversion {
    public:
        explicit TypeSizeConversion() = default;

        ~TypeSizeConversion() = default;

        static auto TypeSize(ColumnType type) -> size_t {
            switch (type) {
                case ColumnType::NUMBER: return 4;
                case ColumnType::DOUBLE: return 8;
                default: return 0;
            }
        }
    };
}
