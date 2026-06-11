//
// Created by huan.yang on 2026-06-11.
//
#include "planner/physical/physical_distinct.h"

#include <vector>

#include "executor/chunk_util.h"

using namespace chickenDB;

auto PhysicalDistinct::Init() -> void {
    Child(0)->Init();
    seen_.clear();
}

auto PhysicalDistinct::Close() -> void {
    Child(0)->Close();
}

// 从孩子拉 chunk，按行键去重（首次出现保留）。cols_ 为空时用全部列做键。
auto PhysicalDistinct::Next() -> Chunk * {
    // 把 cols_（col_id）解析为列下标需要 chunk，故在循环内首批解析。
    while (Chunk *in = Child(0)->Next()) {
        const size_t n = in->Count();
        output_.Init(ChunkUtil::TypesOf(*in), n == 0 ? 1 : n);
        output_.SetColIds(in->ColIds());

        // 解析去重列下标（空 = 全部列）。
        std::vector<size_t> key_cols;
        if (!cols_.empty()) {
            auto col_map = ChunkUtil::BuildColMap(*in);
            for (col_id_t cid : cols_) {
                auto it = col_map.find(cid);
                if (it != col_map.end()) key_cols.push_back(it->second);
            }
        }

        size_t out_row = 0;
        for (size_t r = 0; r < n; r++) {
            std::string key = ChunkUtil::RowKey(*in, r, key_cols);
            if (seen_.insert(key).second) {
                ChunkUtil::CopyRow(output_, out_row, *in, r);
                out_row++;
            }
        }
        if (out_row > 0) {
            output_.SetCount(out_row);
            return &output_;
        }
    }
    return nullptr;
}
