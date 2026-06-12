//
// Created by huan.yang on 2026-06-11.
//
#pragma once
#include <utility>
#include <vector>

#include "common/rid.h"
#include "index/index_key.h"

namespace chickenDB {
    // 索引类别。决定支持的查询能力与存储结构。
    enum class IndexType {
        BPlusTree, // 有序索引：点查 + 范围查
        Hash,      // 哈希索引：仅点查（O(1)），不支持范围
        Bitmap,    // 位图索引：低基数列，点查 + 位运算合并
    };

    // 抽象索引接口。上层（算子、catalog、执行器）只依赖此接口，不依赖具体实现，
    // 以便并存多种索引类别（B+树 / 哈希 / 位图）。key=IndexKey, value=RID，
    // 统一支持非唯一键（同一 key 可对应多个 RID）。
    class Index {
    public:
        virtual ~Index() = default;

        virtual auto Type() const -> IndexType = 0;

        // 是否支持范围查询。哈希索引返回 false。
        virtual auto SupportsRange() const -> bool = 0;

        // 插入 (key, rid)。非唯一索引追加；唯一索引重复 key 应由上层在插入前检查。
        virtual auto Insert(const IndexKey &key, const RID &rid) -> void = 0;

        // 删除某个 (key, rid)。返回是否删到。
        virtual auto Erase(const IndexKey &key, const RID &rid) -> bool = 0;

        // 点查：返回该 key 的全部 RID（无则空）。
        virtual auto Find(const IndexKey &key) const -> std::vector<RID> = 0;

        // 范围查 [lo, hi]（闭区间），按 key 升序返回 RID。
        // 不支持范围的索引（如 Hash）应抛 ChickenException。
        virtual auto Range(const IndexKey &lo, const IndexKey &hi) const -> std::vector<RID> = 0;

        // 全量遍历：返回全部 (key, rid) 对。有序索引按 key 升序，哈希索引顺序不定。
        // 供 IndexOnlyScan 直接从索引读覆盖列（不回表）。
        virtual auto ScanAll() const -> std::vector<std::pair<IndexKey, RID>> = 0;
    };
}
