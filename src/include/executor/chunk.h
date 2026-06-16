//
// Created by huan.yang on 2026-05-11.
//

#pragma once
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "common/enum/statement_type.h"
#include "common/macro.h"
#include "common/types.h"

namespace chickenDB {
    // A Vector is one column's worth of a Chunk. Fixed-width types (NUMBER 4B,
    // DOUBLE 8B) are stored as a flat, cache-friendly array indexed by row.
    // Variable-length types (VARCHAR/VARCHAR2) use an Arrow-style layout: an
    // offsets_ array of size capacity_+1 plus an append-only data_pool_, where
    // row r occupies [offsets_[r], offsets_[r+1]) inside the pool. Both modes
    // share the same validity (null) bitmap. The two storage paths never mix:
    // fixed-width values go through GetValue<T>/SetValue<T>; varlen values go
    // through GetString/AppendString/SetStringAt.
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
            is_var_ = IsVarlen(type);
            if (is_var_) {
                // 变长列：offsets_ 长度 capacity+1，初始全 0（空池）；data_pool_ 预留
                // 经验容量（每行 16 字节）以减少扩容。type_size_ 为 0。
                offsets_ = std::make_unique<int32_t[]>(capacity + 1);
                std::memset(offsets_.get(), 0, (capacity + 1) * sizeof(int32_t));
                data_pool_.clear();
                data_pool_.reserve(capacity * 16);
                append_cursor_ = 0;
                data_.reset();
            } else {
                data_ = std::make_unique<char[]>(type_size_ * capacity);
                offsets_.reset();
                data_pool_.clear();
            }
            // 1 bit per row, rounded up to whole bytes.
            validity_ = std::make_unique<uint8_t[]>((capacity + 7) / 8);
            std::memset(validity_.get(), 0xFF, (capacity + 7) / 8);
        }

        [[nodiscard]] auto GetType() const -> ColumnType { return type_; }
        [[nodiscard]] auto TypeSize() const -> size_t { return type_size_; }
        [[nodiscard]] auto Capacity() const -> size_t { return capacity_; }
        [[nodiscard]] auto IsVar() const -> bool { return is_var_; }

        // Raw column buffer — callers index it as type_size_-strided values.
        // 仅对定长列有意义；变长列请用 GetString/AppendString。
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

        // ---- 变长访问器（仅 VARCHAR/VARCHAR2 合法）----

        // 取第 row 行的字符串视图（指向 data_pool_，Vector 存活期内有效）。
        [[nodiscard]] auto GetString(size_t row) const -> std::string_view {
            const int32_t begin = offsets_[row];
            const int32_t end = offsets_[row + 1];
            return std::string_view(data_pool_.data() + begin,
                                    static_cast<size_t>(end - begin));
        }

        // 按行序追加一个字符串（要求 row 0,1,2... 顺序追加）。写池 + 设 offsets_[row+1]。
        // 返回追加到的行号。这是 SeqScan/Insert/Filter 等顺序填充的主路径。
        auto AppendString(std::string_view s) -> size_t {
            const size_t row = append_cursor_;
            data_pool_.insert(data_pool_.end(), s.begin(), s.end());
            offsets_[row + 1] = static_cast<int32_t>(data_pool_.size());
            append_cursor_ = row + 1;
            return row;
        }

        // 当前已追加的变长行数（仅变长列）。
        [[nodiscard]] auto VarRowCount() const -> size_t { return append_cursor_; }

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

        // 直接以 Arrow 布局批量填充变长列（供反序列化用）：offsets[0..n] + 拼接数据。
        // offsets 必须单调非降、offsets[0]==0。覆盖既有内容。
        auto SetVarColumn(const int32_t *offsets, size_t n, const char *data,
                          size_t data_len) -> void {
            for (size_t i = 0; i <= n; i++) {
                offsets_[i] = offsets[i];
            }
            data_pool_.assign(data, data + data_len);
            append_cursor_ = n;
        }

        // 变长列的 offsets 原始数组（长度 VarRowCount()+1），供序列化读取。
        [[nodiscard]] auto VarOffsets() const -> const int32_t * { return offsets_.get(); }
        [[nodiscard]] auto VarPool() const -> const std::vector<char> & { return data_pool_; }

    private:
        ColumnType type_{ColumnType::NUMBER};
        size_t type_size_{0};
        size_t capacity_{0};
        std::unique_ptr<char[]> data_;
        std::unique_ptr<uint8_t[]> validity_;
        // 变长存储（仅 is_var_ 为真时使用）。
        bool is_var_{false};
        std::unique_ptr<int32_t[]> offsets_;
        std::vector<char> data_pool_;
        size_t append_cursor_{0};
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
