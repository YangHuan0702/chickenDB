//
// Created by huan.yang on 2026-05-11.
//

#pragma once
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

#include "common/enum/statement_type.h"
#include "common/macro.h"
#include "common/types.h"

namespace chickenDB {
    // A Vector is one column's worth of a Chunk: a flat, cache-friendly array of
    // fixed-width values plus a validity (null) bitmap. Variable-length types are
    // out of scope for now — only NUMBER (4B) and DOUBLE (8B) are supported.
    class Vector {
    public:
        Vector() = default;

        Vector(Vector &&) = default;
        Vector &operator=(Vector &&) = default;
        Vector(const Vector &) = delete;
        Vector &operator=(const Vector &) = delete;

        // Allocate storage for `capacity` rows of `type`. Validity starts all-valid.
        auto Init(ColumnType type, size_t capacity) -> void {
            type_ = type;
            capacity_ = capacity;
            type_size_ = TypeSizeConversion::TypeSize(type);
            data_ = std::make_unique<char[]>(type_size_ * capacity);
            // 1 bit per row, rounded up to whole bytes.
            validity_ = std::make_unique<uint8_t[]>((capacity + 7) / 8);
            std::memset(validity_.get(), 0xFF, (capacity + 7) / 8);
        }

        [[nodiscard]] auto GetType() const -> ColumnType { return type_; }
        [[nodiscard]] auto TypeSize() const -> size_t { return type_size_; }
        [[nodiscard]] auto Capacity() const -> size_t { return capacity_; }

        // Raw column buffer — callers index it as type_size_-strided values.
        [[nodiscard]] auto GetData() -> char * { return data_.get(); }
        [[nodiscard]] auto GetData() const -> const char * { return data_.get(); }

        // Typed accessors for fixed-width values.
        template <typename T>
        auto GetValue(size_t row) const -> T {
            T v;
            std::memcpy(&v, data_.get() + row * type_size_, sizeof(T));
            return v;
        }

        template <typename T>
        auto SetValue(size_t row, T v) -> void {
            std::memcpy(data_.get() + row * type_size_, &v, sizeof(T));
        }

        [[nodiscard]] auto IsValid(size_t row) const -> bool {
            return (validity_[row / 8] >> (row % 8)) & 1U;
        }

        auto SetValidity(size_t row, bool valid) -> void {
            uint8_t mask = static_cast<uint8_t>(1U << (row % 8));
            if (valid) {
                validity_[row / 8] |= mask;
            } else {
                validity_[row / 8] &= static_cast<uint8_t>(~mask);
            }
        }

        [[nodiscard]] auto GetValidity() -> uint8_t * { return validity_.get(); }
        [[nodiscard]] auto GetValidity() const -> const uint8_t * { return validity_.get(); }

    private:
        ColumnType type_{ColumnType::NUMBER};
        size_t type_size_{0};
        size_t capacity_{0};
        std::unique_ptr<char[]> data_;
        std::unique_ptr<uint8_t[]> validity_;
    };


    // A Chunk is a horizontal batch of rows in columnar form: one Vector per
    // column, all sharing the same row count. This is the unit of data flow
    // between operators in the vectorized engine.
    class Chunk {
    public:
        explicit Chunk() = default;
        ~Chunk() = default;

        Chunk(const Chunk &) = delete;
        Chunk &operator=(const Chunk &) = delete;
        Chunk(Chunk &&) = default;
        Chunk &operator=(Chunk &&) = default;

        // Lay out one Vector per column type. capacity defaults to K_VECTOR_SIZE.
        auto Init(const std::vector<ColumnType> &types, size_t capacity = K_VECTOR_SIZE) -> void {
            columns_.clear();
            columns_.resize(types.size());
            for (size_t i = 0; i < types.size(); i++) {
                columns_[i].Init(types[i], capacity);
            }
            count_ = 0;
        }

        // Clear the row count without freeing column storage (for reuse).
        auto Reset() -> void { count_ = 0; }

        [[nodiscard]] auto ColumnCount() const -> size_t { return columns_.size(); }
        [[nodiscard]] auto GetColumn(size_t i) -> Vector & { return columns_[i]; }
        [[nodiscard]] auto GetColumn(size_t i) const -> const Vector & { return columns_[i]; }

        [[nodiscard]] auto Count() const -> size_t { return count_; }
        auto SetCount(size_t count) -> void { count_ = count; }

        // 列绑定：每列对应的 schema 全局 col_id，使 chunk 自描述。SeqScan 从 schema
        // 填充，下游算子（Project 等）传播其子集。表达式求值据此把 col_id 映射到列下标。
        [[nodiscard]] auto ColIds() const -> const std::vector<col_id_t> & { return col_ids_; }
        auto SetColIds(std::vector<col_id_t> ids) -> void { col_ids_ = std::move(ids); }

    private:
        size_t count_{0};
        std::vector<Vector> columns_;
        std::vector<col_id_t> col_ids_;
    };
}
