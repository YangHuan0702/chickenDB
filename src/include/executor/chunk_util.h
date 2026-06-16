//
// Created by huan.yang on 2026-06-11.
//
#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "executor/chunk.h"
#include "catalog/schema_version.h"
#include "index/index_key.h"

namespace chickenDB {
    // Chunk 操作辅助：列映射构造、整行拷贝、按 schema 建 chunk。
    // 这些是 Filter/Project/Sort/Join/Aggregate 反复要用的小工具，集中在此避免重复。
    class ChunkUtil {
    public:
        // 从 schema 构造 col_id -> chunk 列下标 的映射（列按 schema 顺序排列）。
        static auto BuildColMap(const SchemaPage *schema)
            -> std::unordered_map<col_id_t, size_t> {
            std::unordered_map<col_id_t, size_t> m;
            for (size_t i = 0; i < schema->columns_.size(); i++) {
                m[schema->columns_[i].col_id] = i;
            }
            return m;
        }

        // 从 chunk 自带的 col_ids 构造 col_id -> 列下标 映射。
        static auto BuildColMap(const Chunk &chunk)
            -> std::unordered_map<col_id_t, size_t> {
            std::unordered_map<col_id_t, size_t> m;
            const auto &ids = chunk.ColIds();
            for (size_t i = 0; i < ids.size(); i++) {
                m[ids[i]] = i;
            }
            return m;
        }

        // 取一个 chunk 各列的类型，用于 Init 一个同构 chunk。
        static auto TypesOf(const Chunk &chunk) -> std::vector<ColumnType> {
            std::vector<ColumnType> types;
            types.reserve(chunk.ColumnCount());
            for (size_t i = 0; i < chunk.ColumnCount(); i++) {
                types.push_back(chunk.GetColumn(i).GetType());
            }
            return types;
        }

        // 把 src 的第 src_row 行，追加为 dst 的第 dst_row 行（按列拷贝值 + validity）。
        // 要求 dst 已 Init 且容量足够，列类型与 src 对应列一致。变长列用 AppendString
        // （要求 dst_row 顺序递增，与算子逐行物化一致）。
        static auto CopyRow(Chunk &dst, size_t dst_row, const Chunk &src, size_t src_row) -> void {
            for (size_t c = 0; c < src.ColumnCount(); c++) {
                const Vector &sv = src.GetColumn(c);
                Vector &dv = dst.GetColumn(c);
                if (dv.IsVar()) {
                    dv.AppendString(sv.GetString(src_row));
                } else {
                    const size_t ts = sv.TypeSize();
                    std::memcpy(dv.GetData() + dst_row * ts, sv.GetData() + src_row * ts, ts);
                }
                dv.SetValidity(dst_row, sv.IsValid(src_row));
            }
        }

        // 把 src 指定列子集的第 src_row 行，拷贝到 dst 的第 dst_row 行。
        // src_cols[i] 是 src 中的列下标，对应 dst 的第 i 列。
        static auto CopyRowProjected(Chunk &dst, size_t dst_row, const Chunk &src, size_t src_row,
                                     const std::vector<size_t> &src_cols) -> void {
            for (size_t i = 0; i < src_cols.size(); i++) {
                const Vector &sv = src.GetColumn(src_cols[i]);
                Vector &dv = dst.GetColumn(i);
                if (dv.IsVar()) {
                    dv.AppendString(sv.GetString(src_row));
                } else {
                    const size_t ts = sv.TypeSize();
                    std::memcpy(dv.GetData() + dst_row * ts, sv.GetData() + src_row * ts, ts);
                }
                dv.SetValidity(dst_row, sv.IsValid(src_row));
            }
        }

        // 把第 row 行指定列子集的原始字节（含 validity 标记）拼成一个 key 字符串，
        // 供 Distinct / 哈希分组按行去重或分组使用。cols 为空表示用全部列。
        // 变长列以 [4字节长度前缀 + 字节] 编码，避免不同切分产生同 key 的歧义。
        static auto RowKey(const Chunk &chunk, size_t row, const std::vector<size_t> &cols)
            -> std::string {
            std::string key;
            auto append_col = [&](size_t c) {
                const Vector &v = chunk.GetColumn(c);
                char valid = v.IsValid(row) ? 1 : 0;
                key.append(&valid, 1);
                if (v.IsVar()) {
                    const std::string_view s = v.GetString(row);
                    const uint32_t len = static_cast<uint32_t>(s.size());
                    key.append(reinterpret_cast<const char *>(&len), sizeof(uint32_t));
                    key.append(s.data(), s.size());
                } else {
                    key.append(v.GetData() + row * v.TypeSize(), v.TypeSize());
                }
            };
            if (cols.empty()) {
                for (size_t c = 0; c < chunk.ColumnCount(); c++) append_col(c);
            } else {
                for (size_t c : cols) append_col(c);
            }
            return key;
        }

        // 取第 row 行在 cols（chunk 列下标）上的值，构造索引键分量。
        // 定长列取 double，变长列(VARCHAR)取字符串。
        static auto MakeIndexKey(const Chunk &chunk, size_t row, const std::vector<size_t> &cols)
            -> std::vector<IndexKeyVal> {
            std::vector<IndexKeyVal> vals;
            vals.reserve(cols.size());
            for (size_t c : cols) {
                const Vector &v = chunk.GetColumn(c);
                if (v.IsVar()) {
                    vals.emplace_back(std::string(v.GetString(row)));
                } else {
                    vals.emplace_back(v.GetType() == ColumnType::NUMBER
                                          ? static_cast<double>(v.GetValue<int32_t>(row))
                                          : v.GetValue<double>(row));
                }
            }
            return vals;
        }
    };
}
