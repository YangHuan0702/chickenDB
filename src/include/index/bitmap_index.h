//
// Created by huan.yang on 2026-06-11.
//
#pragma once
#include <map>
#include <vector>

#include "index/index.h"

namespace chickenDB {
    // 位图索引：Index 接口实现，面向低基数列（如状态、性别、枚举）。
    // 每个不同的 key 对应一组命中行（RID 列表，概念上即该值的 bitmap）。
    // 用有序 map<IndexKey, RID列表> 承载，故点查与范围查（按 key 区间 OR 各 bitmap）
    // 均可支持。低基数下空间高效、多条件可做 bitmap AND/OR 合并（后续）。
    class BitmapIndex : public Index {
    public:
        explicit BitmapIndex() = default;
        ~BitmapIndex() override = default;

        auto Type() const -> IndexType override;
        auto SupportsRange() const -> bool override; // true

        auto Insert(const IndexKey &key, const RID &rid) -> void override;
        auto Erase(const IndexKey &key, const RID &rid) -> bool override;
        auto Find(const IndexKey &key) const -> std::vector<RID> override;
        auto Range(const IndexKey &lo, const IndexKey &hi) const -> std::vector<RID> override;
        auto ScanAll() const -> std::vector<std::pair<IndexKey, RID>> override;

    private:
        // 有序映射：key -> 该值命中的全部 RID（该值的 bitmap）。
        std::map<IndexKey, std::vector<RID>> buckets_;
    };
}
