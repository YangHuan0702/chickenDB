//
// Created by huan.yang on 2026-06-11.
//
#include "index/b_plus_tree.h"

#include <algorithm>

using namespace chickenDB;

BPlusTreeIndex::BPlusTreeIndex(size_t order) : order_(order) {
    root_ = std::make_unique<Node>(true);
}

auto BPlusTreeIndex::Type() const -> IndexType { return IndexType::BPlusTree; }

auto BPlusTreeIndex::SupportsRange() const -> bool { return true; }

// 插入 (key, rid)。非唯一：同 key 追加 rid。
auto BPlusTreeIndex::Insert(const IndexKey &key, const RID &rid) -> void {
    Node *leaf = FindLeaf(key);
    auto it = std::lower_bound(leaf->keys_.begin(), leaf->keys_.end(), key);
    size_t pos = it - leaf->keys_.begin();
    if (it != leaf->keys_.end() && *it == key) {
        leaf->rids_[pos].push_back(rid);
        return;
    }
    leaf->keys_.insert(it, key);
    leaf->rids_.insert(leaf->rids_.begin() + pos, std::vector<RID>{rid});

    if (leaf->keys_.size() > order_) {
        SplitLeaf(leaf);
    }
}

// 点查：返回该 key 的全部 RID（无则空）。
auto BPlusTreeIndex::Find(const IndexKey &key) const -> std::vector<RID> {
    Node *leaf = FindLeaf(key);
    auto it = std::lower_bound(leaf->keys_.begin(), leaf->keys_.end(), key);
    if (it != leaf->keys_.end() && *it == key) {
        return leaf->rids_[it - leaf->keys_.begin()];
    }
    return {};
}

// 范围查 [lo, hi]（闭区间）：按 key 升序返回区间内全部 RID。
auto BPlusTreeIndex::Range(const IndexKey &lo, const IndexKey &hi) const -> std::vector<RID> {
    std::vector<RID> out;
    Node *leaf = FindLeaf(lo);
    while (leaf != nullptr) {
        for (size_t i = 0; i < leaf->keys_.size(); i++) {
            if (leaf->keys_[i] < lo) continue;
            if (leaf->keys_[i] > hi) return out;
            out.insert(out.end(), leaf->rids_[i].begin(), leaf->rids_[i].end());
        }
        leaf = leaf->next_;
    }
    return out;
}

// 全量遍历：从最左叶子沿 next_ 链表收集全部 (key, rid)，按 key 升序。
auto BPlusTreeIndex::ScanAll() const -> std::vector<std::pair<IndexKey, RID>> {
    std::vector<std::pair<IndexKey, RID>> out;
    Node *cur = root_.get();
    while (cur != nullptr && !cur->is_leaf_) {
        cur = cur->children_.empty() ? nullptr : cur->children_[0].get();
    }
    while (cur != nullptr) {
        for (size_t i = 0; i < cur->keys_.size(); i++) {
            for (const RID &rid : cur->rids_[i]) {
                out.emplace_back(cur->keys_[i], rid);
            }
        }
        cur = cur->next_;
    }
    return out;
}

// 删除某个 (key, rid)。返回是否删到。内存版不做下溢合并（不影响正确性，仅空间）。
auto BPlusTreeIndex::Erase(const IndexKey &key, const RID &rid) -> bool {
    Node *leaf = FindLeaf(key);
    auto it = std::lower_bound(leaf->keys_.begin(), leaf->keys_.end(), key);
    if (it == leaf->keys_.end() || !(*it == key)) return false;
    size_t pos = it - leaf->keys_.begin();
    auto &rids = leaf->rids_[pos];
    auto rit = std::find(rids.begin(), rids.end(), rid);
    if (rit == rids.end()) return false;
    rids.erase(rit);
    if (rids.empty()) {
        leaf->keys_.erase(leaf->keys_.begin() + pos);
        leaf->rids_.erase(leaf->rids_.begin() + pos);
    }
    return true;
}

auto BPlusTreeIndex::FindLeaf(const IndexKey &key) const -> Node * {
    Node *cur = root_.get();
    while (!cur->is_leaf_) {
        // 找第一个 > key 的分隔键，走对应孩子。
        auto it = std::upper_bound(cur->keys_.begin(), cur->keys_.end(), key);
        size_t idx = it - cur->keys_.begin();
        cur = cur->children_[idx].get();
    }
    return cur;
}

auto BPlusTreeIndex::SplitLeaf(Node *leaf) -> void {
    const size_t mid = leaf->keys_.size() / 2;
    auto sibling = std::make_unique<Node>(true);
    sibling->keys_.assign(leaf->keys_.begin() + mid, leaf->keys_.end());
    sibling->rids_.assign(leaf->rids_.begin() + mid, leaf->rids_.end());
    leaf->keys_.resize(mid);
    leaf->rids_.resize(mid);

    sibling->next_ = leaf->next_;
    leaf->next_ = sibling.get();

    IndexKey up_key = sibling->keys_.front();
    InsertIntoParent(leaf, up_key, std::move(sibling));
}

auto BPlusTreeIndex::InsertIntoParent(Node *left, const IndexKey &key,
                                      std::unique_ptr<Node> right) -> void {
    if (left == root_.get()) {
        auto new_root = std::make_unique<Node>(false);
        new_root->keys_.push_back(key);
        left->parent_ = new_root.get();
        right->parent_ = new_root.get();
        new_root->children_.push_back(std::move(root_));
        new_root->children_.push_back(std::move(right));
        root_ = std::move(new_root);
        return;
    }
    Node *parent = left->parent_;
    auto it = std::upper_bound(parent->keys_.begin(), parent->keys_.end(), key);
    size_t pos = it - parent->keys_.begin();
    right->parent_ = parent;
    parent->keys_.insert(parent->keys_.begin() + pos, key);
    parent->children_.insert(parent->children_.begin() + pos + 1, std::move(right));

    if (parent->keys_.size() > order_) {
        SplitInternal(parent);
    }
}

auto BPlusTreeIndex::SplitInternal(Node *node) -> void {
    const size_t mid = node->keys_.size() / 2;
    IndexKey up_key = node->keys_[mid];

    auto sibling = std::make_unique<Node>(false);
    sibling->keys_.assign(node->keys_.begin() + mid + 1, node->keys_.end());
    for (size_t i = mid + 1; i < node->children_.size(); i++) {
        node->children_[i]->parent_ = sibling.get();
        sibling->children_.push_back(std::move(node->children_[i]));
    }
    node->keys_.resize(mid);
    node->children_.resize(mid + 1);

    InsertIntoParent(node, up_key, std::move(sibling));
}
