//
// Created by huan.yang on 2026-01-28.
//
#pragma once
#include <cstdint>
#include "common/constants.h"

namespace chickenDB {
    struct blob_t {
        data_ptr_t data;
        index_t size;
    };

    //===--------------------------------------------------------------------===//
    // Internal Types
    //===--------------------------------------------------------------------===//
    enum class  TypeId : uint8_t {
        INVALID = 0,
        BOOLEAN = 1,   /* bool */
        TINYINT = 2,   /* int8_t */
        SMALLINT = 3,  /* int16_t */
        INTEGER = 4,   /* int32_t */
        BIGINT = 5,    /* int64_t */
        HASH = 6,      /* uint64_t */
        POINTER = 7,   /* uintptr_t */
        FLOAT = 8,     /* float32_t */
        DOUBLE = 9,    /* float64_t */
        VARCHAR = 10,  /* char*, representing a null-terminated UTF-8 string */
        VARBINARY = 11 /* blob_t, representing arbitrary bytes */
    };

    //===--------------------------------------------------------------------===//
    // SQL Types
    //===--------------------------------------------------------------------===//
    enum class SQLTypeId : uint8_t {
        INVALID = 0,
        SQLNULL = 1,
        BOOLEAN = 2,
        TINYINT = 3,
        SMALLINT = 4,
        INTEGER = 5,
        BIGINT = 6,
        DATE = 7,
        TIMESTAMP = 8,
        REAL = 9,
        DOUBLE = 10,
        FLOAT = 11,
        DECIMAL = 12,
        CHAR = 13,
        VARCHAR = 14,
        VARBINARY = 15
    };


    struct SQLType {
        SQLTypeId id;
        uint16_t width;	// 数据类型的宽度（通常表示字节数）
        uint8_t scale;	 // 数据类型的小数位数（仅对某些类型有效，如定点数、浮点数等）

        SQLType(SQLTypeId id = SQLTypeId::INVALID, uint16_t width = 0, uint8_t scale = 0)
            : id(id), width(width), scale(scale) {
        }

        bool operator==(const SQLType &rhs) const {
            return id == rhs.id && width == rhs.width && scale == rhs.scale;
        }
        bool operator!=(const SQLType &rhs) const {
            return !(*this == rhs);
        }
    };



}
