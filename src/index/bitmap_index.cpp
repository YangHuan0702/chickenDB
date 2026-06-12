//
// Created by huan.yang on 2026-06-11.
//
#include "index/bitmap_index.h"

#include <algorithm>

using namespace chickenDB;

auto BitmapIndex::Type() const -> IndexType { return IndexType::Bitmap; }

auto BitmapIndex::SupportsRange() const -> bool { return true; }

auto BitmapIndex::Insert(const IndexKey &key, const RID &rid) -> void {
    buckets_[key].push_back(rid);
}

auto BitmapIndex::Erase(const IndexKey &key, const RID &rid) -> bool {
    auto it = buckets_.find(key);
    if (it == buckets_.end()) return false;
    auto &rids = it->second;
    auto rit = std::find(rids.begin(), rids.end(), rid);
    if (rit == rids.end()) return false;
    rids.erase(rit);
    if (rids.empty()) {
        buckets_.erase(it);
    }
    return true;
}

auto BitmapIndex::Find(const IndexKey &key) const -> std::vector<RID> {
    auto it = buckets_.find(key);
    return it == buckets_.end() ? std::vector<RID>{} : it->second;
}

// 范围查：按 key 升序 OR 各命中值的 bitmap（这里即拼接各桶的 RID）。
auto BitmapIndex::Range(const IndexKey &lo, const IndexKey &hi) const -> std::vector<RID> {
    std::vector<RID> out;
    // std::map 有序：lower_bound(lo) 起，直到 key > hi。
    for (auto it = buckets_.lower_bound(lo); it != buckets_.end(); ++it) {
        if (it->first > hi) break;
        out.insert(out.end(), it->second.begin(), it->second.end());
    }
    return out;
}

auto BitmapIndex::ScanAll() const -> std::vector<std::pair<IndexKey, RID>> {
    std::vector<std::pair<IndexKey, RID>> out;
    for (const auto &kv : buckets_) { // std::map 有序，按 key 升序
        for (const RID &rid : kv.second) {
            out.emplace_back(kv.first, rid);
        }
    }
    return out;
}
