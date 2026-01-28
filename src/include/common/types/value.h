//
// Created by huan.yang on 2026-01-28.
//

#pragma once
#include <cstdint>
#include <stdexcept>
#include <string>
#include "common/types/types.h"

namespace chickenDB {
    class Value {
        friend class Vector;

    public:
        // 创建指定类型的空 NULL 值。
        Value(TypeId type = TypeId::INTEGER) : type(type), is_null(true) {
        }

        // 创建一个BIGINT值
        Value(int32_t val) : type(TypeId::INTEGER), is_null(false) {
            value_.integer = val;
        }

        // 创建一个FLOAT 值
        Value(float val) : type(TypeId::FLOAT), is_null(false) {
            value_.float_ = val;
        }

        // 创建一个DOUBLE 值
        Value(double val) : type(TypeId::DOUBLE), is_null(false) {
            value_.double_ = val;
        }

        // 创建一个VARCHAR值
        Value(const char *val) : Value(val ? std::string(val) : std::string()) {
        }

        Value(std::string str) : type(TypeId::VARCHAR), is_null(str.length() > 0), str_value(str) {
        }

        Value(const Value &other);


        // 创建给定类型的最小值（仅限数值类型）。
        static Value MinimumValue(TypeId type);

        // 创建给定类型的最大值（仅限数值类型）
        static Value MaximumValue(TypeId type);

        // 创建一个指定类型的数值，并赋予指定的值。
        static Value Numeric(TypeId id, int64_t value);

        // 从指定的值创建一个 tinyint 类型的 Value。
        static Value BOOLEAN(int8_t value);

        // 从指定的值创建一个 tinyint 类型的 Value。
        static Value TINYINT(int8_t value);

        //  从指定的值创建一个 smallint 类型的 Value。
        static Value SMALLINT(int16_t value);

        // 从指定的值创建一个 integer 类型的 Value。
        static Value INTEGER(int32_t value);

        // 从指定的值创建一个 bigint 类型的 Value。
        static Value BIGINT(int64_t value);

        // 从指定的值创建一个 hash 类型的 Value。
        static Value HASH(uint64_t value);

        // 从指定的值创建一个指针类型的 Value。
        static Value POINTER(uintptr_t value);


        template<class T>
        static Value CreateValue(T value) {
            throw std::runtime_error("Unimplemented template type for value creation");
        }

        int64_t GetNumericValue();

        // 返回该值的副本。
        Value Copy() const {
            return Value(*this);
        }

        // 将该值转换为另一种类型
        Value CastAs(TypeId target_type) const;

        // 将该值转换为另一种类型
        Value CastAs(SQLType source_type, SQLType target_type);

        // 该值的类型。
        TypeId type;
        // 该值是否为 NULL。
        bool is_null;

        // 如果该对象是常量大小类型，则为该值的值。
        union Val {
            int8_t boolean;
            int8_t tinyint;
            int16_t smallint;
            int32_t integer;
            int64_t bigint;
            float float_;
            double double_;
            uintptr_t pointer;
            uint64_t hash;
        } value_;

        // 如果该对象是可变大小类型，则为该值的值。
        std::string str_value;


        //===--------------------------------------------------------------------===//
        // Numeric Operators
        //===--------------------------------------------------------------------===//
        Value operator+(const Value &rhs) const;

        Value operator-(const Value &rhs) const;

        Value operator*(const Value &rhs) const;

        Value operator/(const Value &rhs) const;

        Value operator%(const Value &rhs) const;

        //===--------------------------------------------------------------------===//
        // Comparison Operators
        //===--------------------------------------------------------------------===//
        bool operator==(const Value &rhs) const;

        bool operator!=(const Value &rhs) const;

        bool operator<(const Value &rhs) const;

        bool operator>(const Value &rhs) const;

        bool operator<=(const Value &rhs) const;

        bool operator>=(const Value &rhs) const;

        bool operator==(const int64_t &rhs) const;

        bool operator!=(const int64_t &rhs) const;

        bool operator<(const int64_t &rhs) const;

        bool operator>(const int64_t &rhs) const;

        bool operator<=(const int64_t &rhs) const;

        bool operator>=(const int64_t &rhs) const;

        // 如果这些值（大致上）相等，则返回 true。请注意，这并不是 SQL 等价性。对于此函数，NULL 值被认为是等价的，且接近的浮动点值也被认为是等价的。
        static bool ValuesAreEqual(Value result_value, Value value);


        void Print();

    private:
        // 用于类型转换的模板辅助函数。
        template<class DST, class OP>
        static DST _cast(const Value &v);

        // 用于二进制操作的模板辅助函数。
        template<class OP>
        static void _templated_binary_operation(const Value &left, const Value &right, Value &result, bool ignore_null);

        // 用于布尔操作的模板辅助函数。
        template<class OP>
        static bool _templated_boolean_operation(const Value &left, const Value &right);
    };
}
