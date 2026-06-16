//
// Created by huan.yang on 2026-06-11.
//
#pragma once
#include <vector>

#include "executor/chunk.h"
#include "executor/chunk_util.h"
#include "common/chicken_execption.h"

namespace chickenDB {
    // 连接算子共用的物化/输出辅助。等值连接，输出 = 左列拼右列；定长列值统一取 double。
    struct JoinRows {
        std::vector<std::vector<double>> rows;
        std::vector<ColumnType> types;
        std::vector<col_id_t> col_ids;
        std::vector<size_t> key_idx;
    };

    class JoinUtil {
    public:
        // 把一侧（child）的全部输出物化为行集合，并解析 key 列下标。
        static auto Materialize(PhysicalOperator *child, const std::vector<col_id_t> &keys)
            -> JoinRows {
            JoinRows r;
            bool resolved = false;
            while (Chunk *in = child->Next()) {
                if (!resolved) {
                    r.types = ChunkUtil::TypesOf(*in);
                    r.col_ids = in->ColIds();
                    // 连接当前把行物化为 double，不支持变长(VARCHAR)列；含变长列报错而非
                    // 静默损坏（数值列连接不受影响）。
                    for (const auto &t : r.types) {
                        ChickenException::AssertCondition(!IsVarlen(t),
                            "[Join] varchar columns in join not supported yet");
                    }
                    auto col_map = ChunkUtil::BuildColMap(*in);
                    for (col_id_t cid : keys) {
                        auto it = col_map.find(cid);
                        ChickenException::AssertCondition(it != col_map.end(), "[Join] key column not found");
                        r.key_idx.push_back(it->second);
                    }
                    resolved = true;
                }
                const size_t n = in->Count();
                const size_t cols = in->ColumnCount();
                for (size_t row = 0; row < n; row++) {
                    std::vector<double> vals(cols);
                    for (size_t c = 0; c < cols; c++) {
                        const Vector &v = in->GetColumn(c);
                        vals[c] = v.GetType() == ColumnType::NUMBER
                                      ? static_cast<double>(v.GetValue<int32_t>(row))
                                      : v.GetValue<double>(row);
                    }
                    r.rows.push_back(std::move(vals));
                }
            }
            return r;
        }

        // 判断左行 lrow 与右行 rrow 在各 key 上是否相等。
        static auto KeysEqual(const std::vector<double> &lrow, const std::vector<size_t> &lkey,
                              const std::vector<double> &rrow, const std::vector<size_t> &rkey) -> bool {
            for (size_t i = 0; i < lkey.size(); i++) {
                if (lrow[lkey[i]] != rrow[rkey[i]]) return false;
            }
            return true;
        }

        // 取一行在 key 列上的值序列（供哈希索引建键）。
        static auto KeyVals(const std::vector<double> &row, const std::vector<size_t> &key_idx)
            -> std::vector<double> {
            std::vector<double> vals;
            vals.reserve(key_idx.size());
            for (size_t idx : key_idx) vals.push_back(row[idx]);
            return vals;
        }

        // 构造拼接输出 chunk 的列类型与 col_ids（左列在前，右列在后）。
        static auto BuildOutputSchema(const std::vector<ColumnType> &lt, const std::vector<col_id_t> &lid,
                                      const std::vector<ColumnType> &rt, const std::vector<col_id_t> &rid,
                                      std::vector<ColumnType> &out_types, std::vector<col_id_t> &out_ids) -> void {
            out_types = lt;
            out_types.insert(out_types.end(), rt.begin(), rt.end());
            out_ids = lid;
            out_ids.insert(out_ids.end(), rid.begin(), rid.end());
        }

        // 把左行 + 右行写入 output 的第 row 行（左列在前）。
        static auto EmitJoined(Chunk &output, size_t row, const std::vector<double> &lrow,
                               const std::vector<double> &rrow, const std::vector<ColumnType> &out_types) -> void {
            const size_t lcols = lrow.size();
            for (size_t c = 0; c < out_types.size(); c++) {
                double val = c < lcols ? lrow[c] : rrow[c - lcols];
                if (out_types[c] == ColumnType::NUMBER) {
                    output.GetColumn(c).SetValue<int32_t>(row, static_cast<int32_t>(val));
                } else {
                    output.GetColumn(c).SetValue<double>(row, val);
                }
            }
        }
    };
}
