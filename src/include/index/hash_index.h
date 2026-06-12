//
// Created by huan.yang on 2026-06-11.
//
#pragma once
#include <unordered_map>
#include <vector>

#include "index/index.h"

namespace chickenDB {
    // 哈希索引：Index 接口的等值索引实现。点查 O(1)，不支持范围查询。
    // 适合等值过滤（WHERE x = ?）和哈希连接的探测侧。
    class HashIndex : public Index {
    public:
        explicit HashIndex() = default;
        ~HashIndex() override = default;

        auto Type() const -> IndexType override;
        auto SupportsRange() const -> bool override; // false

        auto Insert(const IndexKey &key, const RID &rid) -> void override;
        auto Erase(const IndexKey &key, const RID &rid) -> bool override;
        auto Find(const IndexKey &key) const -> std::vector<RID> override;
        // 不支持：调用即抛 ChickenException。
        auto Range(const IndexKey &lo, const IndexKey &hi) const -> std::vector<RID> override;
        auto ScanAll() const -> std::vector<std::pair<IndexKey, RID>> override;

    private:
        // IndexKey 的哈希器：对各列 double 的 bit 表示做组合哈希。
        struct KeyHash {
            auto operator()(const IndexKey &k) const -> size_t;
        };
        std::unordered_map<IndexKey, std::vector<RID>, KeyHash> table_;
    };
}
