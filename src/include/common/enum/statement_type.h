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

    // 变长（可变宽度）列判定。VARCHAR/VARCHAR2 在执行引擎用 Arrow 风格 offset+pool
    // 存储，而非定宽 slot，故 TypeSize 对它们返回 0；所有按 TypeSize 做定长跨步的
    // 代码路径都应先用此 helper 分流到变长处理。
    inline auto IsVarlen(ColumnType type) -> bool {
        return type == ColumnType::VARCHAR || type == ColumnType::VARCHAR2;
    }

    class TypeSizeConversion {
    public:
        explicit TypeSizeConversion() = default;

        ~TypeSizeConversion() = default;

        // 定长类型返回字节宽度；变长类型（VARCHAR/VARCHAR2）返回 0。返回 0 即表示
        // 该列不能按固定跨步寻址，调用方须用 IsVarlen 走变长路径。
        static auto TypeSize(ColumnType type) -> size_t {
            switch (type) {
                case ColumnType::NUMBER: return 4;
                case ColumnType::DOUBLE: return 8;
                default: return 0;
            }
        }
    };
}
