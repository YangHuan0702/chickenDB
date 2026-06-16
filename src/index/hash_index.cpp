//
// Created by huan.yang on 2026-06-11.
//
#include "index/hash_index.h"

#include <algorithm>
#include <functional>

#include "common/chicken_execption.h"

using namespace chickenDB;

auto HashIndex::KeyHash::operator()(const IndexKey &k) const -> size_t {
    size_t h = 1469598103934665603ULL; // FNV offset basis
    std::hash<double> dh;
    std::hash<std::string> sh;
    for (const IndexKeyVal &v : k.vals) {
        h ^= v.is_str ? sh(v.str) : dh(v.num);
        h *= 1099511628211ULL; // FNV prime
    }
    return h;
}

auto HashIndex::Type() const -> IndexType { return IndexType::Hash; }

auto HashIndex::SupportsRange() const -> bool { return false; }

auto HashIndex::Insert(const IndexKey &key, const RID &rid) -> void {
    table_[key].push_back(rid);
}

auto HashIndex::Erase(const IndexKey &key, const RID &rid) -> bool {
    auto it = table_.find(key);
    if (it == table_.end()) return false;
    auto &rids = it->second;
    auto rit = std::find(rids.begin(), rids.end(), rid);
    if (rit == rids.end()) return false;
    rids.erase(rit);
    if (rids.empty()) {
        table_.erase(it);
    }
    return true;
}

auto HashIndex::Find(const IndexKey &key) const -> std::vector<RID> {
    auto it = table_.find(key);
    return it == table_.end() ? std::vector<RID>{} : it->second;
}

auto HashIndex::Range(const IndexKey &, const IndexKey &) const -> std::vector<RID> {
    throw ChickenException("[HashIndex] range query not supported by hash index");
}

auto HashIndex::ScanAll() const -> std::vector<std::pair<IndexKey, RID>> {
    std::vector<std::pair<IndexKey, RID>> out;
    for (const auto &kv : table_) {
        for (const RID &rid : kv.second) {
            out.emplace_back(kv.first, rid);
        }
    }
    return out;
}
