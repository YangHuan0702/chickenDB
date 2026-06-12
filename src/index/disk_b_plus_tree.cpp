//
// Created by huan.yang on 2026-06-11.
//
#include "index/disk_b_plus_tree.h"

#include <cstring>

#include "buffer/page.h"
#include "common/constants.h"
#include "common/enum/page_type.h"

using namespace chickenDB;

namespace {
    // 页内固定偏移。
    constexpr size_t OFF_MAGIC = 0;       // u32
    constexpr size_t OFF_PAGETYPE = 4;    // u8
    constexpr size_t OFF_IS_LEAF = 5;     // u8
    constexpr size_t OFF_NUM_KEYS = 6;    // u16
    constexpr size_t OFF_NEXT_LEAF = 8;   // int64
    constexpr size_t OFF_KEYS = 16;       // 键区起点
}

DiskBPlusTreeIndex::DiskBPlusTreeIndex(std::shared_ptr<BufferManager> buffer, table_id_t table_id,
                                       size_t key_cols, page_id_t root_page_id)
    : buffer_(std::move(buffer)), table_id_(table_id), key_cols_(key_cols), root_page_id_(root_page_id) {
    if (root_page_id_ < 0) {
        // 新建索引：分配一个空叶子作为根。
        root_page_id_ = AllocNode(/*is_leaf=*/true);
    }
}

auto DiskBPlusTreeIndex::LeafSlots() const -> uint16_t {
    // OFF_KEYS + slots*(keyBytes + ridBytes) <= PAGE_SIZE
    const size_t rid_bytes = sizeof(int64_t) + sizeof(uint32_t);
    const size_t per = KeyBytes() + rid_bytes;
    return static_cast<uint16_t>((PAGE_SIZE - OFF_KEYS) / per);
}

auto DiskBPlusTreeIndex::InternalSlots() const -> uint16_t {
    // OFF_KEYS + slots*keyBytes + (slots+1)*8 <= PAGE_SIZE
    const size_t child_bytes = sizeof(int64_t);
    const size_t avail = PAGE_SIZE - OFF_KEYS - child_bytes;
    return static_cast<uint16_t>(avail / (KeyBytes() + child_bytes));
}

// 逻辑容量 = 物理 slot - 1，预留 1 个给“先插入后分裂”的临时溢出。
auto DiskBPlusTreeIndex::LeafCapacity() const -> uint16_t {
    return static_cast<uint16_t>(LeafSlots() - 1);
}

auto DiskBPlusTreeIndex::InternalCapacity() const -> uint16_t {
    return static_cast<uint16_t>(InternalSlots() - 1);
}

auto DiskBPlusTreeIndex::ReadHeader(const char *data, NodeView &v, page_id_t page_no) const -> void {
    v.page_no = page_no;
    uint8_t leaf = 0;
    std::memcpy(&leaf, data + OFF_IS_LEAF, sizeof(uint8_t));
    v.is_leaf = leaf != 0;
    std::memcpy(&v.num_keys, data + OFF_NUM_KEYS, sizeof(uint16_t));
    int64_t next = -1;
    std::memcpy(&next, data + OFF_NEXT_LEAF, sizeof(int64_t));
    v.next_leaf = next;
}

auto DiskBPlusTreeIndex::WriteHeader(char *data, const NodeView &v) const -> void {
    uint32_t magic = static_cast<uint32_t>(PAGE_MAGIC_NUM);
    uint8_t ptype = static_cast<uint8_t>(PageType::INDEX);
    uint8_t leaf = v.is_leaf ? 1 : 0;
    std::memcpy(data + OFF_MAGIC, &magic, sizeof(uint32_t));
    std::memcpy(data + OFF_PAGETYPE, &ptype, sizeof(uint8_t));
    std::memcpy(data + OFF_IS_LEAF, &leaf, sizeof(uint8_t));
    std::memcpy(data + OFF_NUM_KEYS, &v.num_keys, sizeof(uint16_t));
    int64_t next = v.next_leaf;
    std::memcpy(data + OFF_NEXT_LEAF, &next, sizeof(int64_t));
}

auto DiskBPlusTreeIndex::KeyAt(const char *data, uint16_t i) const -> IndexKey {
    std::vector<double> vals(key_cols_);
    const char *p = data + OFF_KEYS + static_cast<size_t>(i) * KeyBytes();
    std::memcpy(vals.data(), p, KeyBytes());
    return IndexKey(std::move(vals));
}

auto DiskBPlusTreeIndex::SetKeyAt(char *data, uint16_t i, const IndexKey &k) const -> void {
    char *p = data + OFF_KEYS + static_cast<size_t>(i) * KeyBytes();
    std::memcpy(p, k.vals.data(), KeyBytes());
}

auto DiskBPlusTreeIndex::LeafRidAt(const char *data, uint16_t i) const -> RID {
    // RID 区在键区之后。
    const size_t rid_base = OFF_KEYS + static_cast<size_t>(LeafSlots()) * KeyBytes();
    const size_t rid_bytes = sizeof(int64_t) + sizeof(uint32_t);
    const char *p = data + rid_base + static_cast<size_t>(i) * rid_bytes;
    int64_t page_no; uint32_t row;
    std::memcpy(&page_no, p, sizeof(int64_t));
    std::memcpy(&row, p + sizeof(int64_t), sizeof(uint32_t));
    return RID(page_no, row);
}

auto DiskBPlusTreeIndex::SetLeafRidAt(char *data, uint16_t i, const RID &rid) const -> void {
    const size_t rid_base = OFF_KEYS + static_cast<size_t>(LeafSlots()) * KeyBytes();
    const size_t rid_bytes = sizeof(int64_t) + sizeof(uint32_t);
    char *p = data + rid_base + static_cast<size_t>(i) * rid_bytes;
    int64_t page_no = rid.page_no; uint32_t row = rid.row_idx;
    std::memcpy(p, &page_no, sizeof(int64_t));
    std::memcpy(p + sizeof(int64_t), &row, sizeof(uint32_t));
}

auto DiskBPlusTreeIndex::ChildAt(const char *data, uint16_t i) const -> page_id_t {
    const size_t child_base = OFF_KEYS + static_cast<size_t>(InternalSlots()) * KeyBytes();
    const char *p = data + child_base + static_cast<size_t>(i) * sizeof(int64_t);
    int64_t child;
    std::memcpy(&child, p, sizeof(int64_t));
    return child;
}

auto DiskBPlusTreeIndex::SetChildAt(char *data, uint16_t i, page_id_t child) const -> void {
    const size_t child_base = OFF_KEYS + static_cast<size_t>(InternalSlots()) * KeyBytes();
    char *p = data + child_base + static_cast<size_t>(i) * sizeof(int64_t);
    int64_t c = child;
    std::memcpy(p, &c, sizeof(int64_t));
}

auto DiskBPlusTreeIndex::AllocNode(bool is_leaf) -> page_id_t {
    Page *page = buffer_->NewPage(table_id_);
    const page_id_t no = page->page_id_.page_no;
    std::memset(page->data, 0, PAGE_SIZE);
    NodeView v{no, is_leaf, 0, -1};
    WriteHeader(page->data, v);
    buffer_->UnpinPage(table_id_, no, /*is_dirty=*/true);
    return no;
}

// 下行到 key 所属叶子页号。
auto DiskBPlusTreeIndex::FindLeafPage(const IndexKey &key) const -> page_id_t {
    page_id_t cur = root_page_id_;
    while (true) {
        Page *page = buffer_->FetchPage(table_id_, cur);
        NodeView v{};
        ReadHeader(page->data, v, cur);
        if (v.is_leaf) {
            buffer_->UnpinPage(table_id_, cur, false);
            return cur;
        }
        // 找第一个 > key 的分隔键，走对应孩子。
        uint16_t idx = 0;
        while (idx < v.num_keys && !(key < KeyAt(page->data, idx))) idx++;
        page_id_t child = ChildAt(page->data, idx);
        buffer_->UnpinPage(table_id_, cur, false);
        cur = child;
    }
}

auto DiskBPlusTreeIndex::Find(const IndexKey &key) const -> std::vector<RID> {
    std::vector<RID> out;
    page_id_t leaf = FindLeafPage(key);
    // 非唯一键可能跨叶子，沿链表收集所有相等键。
    while (leaf >= 0) {
        Page *page = buffer_->FetchPage(table_id_, leaf);
        NodeView v{};
        ReadHeader(page->data, v, leaf);
        bool past = false;
        for (uint16_t i = 0; i < v.num_keys; i++) {
            IndexKey k = KeyAt(page->data, i);
            if (k < key) continue;
            if (key < k) { past = true; break; }
            out.push_back(LeafRidAt(page->data, i));
        }
        page_id_t next = v.next_leaf;
        buffer_->UnpinPage(table_id_, leaf, false);
        if (past) break;
        leaf = next;
    }
    return out;
}

auto DiskBPlusTreeIndex::Range(const IndexKey &lo, const IndexKey &hi) const -> std::vector<RID> {
    std::vector<RID> out;
    page_id_t leaf = FindLeafPage(lo);
    while (leaf >= 0) {
        Page *page = buffer_->FetchPage(table_id_, leaf);
        NodeView v{};
        ReadHeader(page->data, v, leaf);
        bool done = false;
        for (uint16_t i = 0; i < v.num_keys; i++) {
            IndexKey k = KeyAt(page->data, i);
            if (k < lo) continue;
            if (hi < k) { done = true; break; }
            out.push_back(LeafRidAt(page->data, i));
        }
        page_id_t next = v.next_leaf;
        buffer_->UnpinPage(table_id_, leaf, false);
        if (done) break;
        leaf = next;
    }
    return out;
}

auto DiskBPlusTreeIndex::ScanAll() const -> std::vector<std::pair<IndexKey, RID>> {
    std::vector<std::pair<IndexKey, RID>> out;
    // 下行到最左叶子。
    page_id_t cur = root_page_id_;
    while (true) {
        Page *page = buffer_->FetchPage(table_id_, cur);
        NodeView v{};
        ReadHeader(page->data, v, cur);
        if (v.is_leaf) { buffer_->UnpinPage(table_id_, cur, false); break; }
        page_id_t child = ChildAt(page->data, 0);
        buffer_->UnpinPage(table_id_, cur, false);
        cur = child;
    }
    // 沿叶子链表收集。
    while (cur >= 0) {
        Page *page = buffer_->FetchPage(table_id_, cur);
        NodeView v{};
        ReadHeader(page->data, v, cur);
        for (uint16_t i = 0; i < v.num_keys; i++) {
            out.emplace_back(KeyAt(page->data, i), LeafRidAt(page->data, i));
        }
        page_id_t next = v.next_leaf;
        buffer_->UnpinPage(table_id_, cur, false);
        cur = next;
    }
    return out;
}

// 在叶子有序位置插入 (key, rid)。调用前确保叶子有空位。
auto DiskBPlusTreeIndex::InsertIntoLeaf(page_id_t leaf, const IndexKey &key, const RID &rid) -> void {
    Page *page = buffer_->FetchPage(table_id_, leaf);
    NodeView v{};
    ReadHeader(page->data, v, leaf);
    // 找插入位置（第一个 >= key 的位置，等键追加到该区间末尾以保留插入序）。
    uint16_t pos = 0;
    while (pos < v.num_keys && !(key < KeyAt(page->data, pos))) pos++;
    // 后移腾位。
    for (uint16_t i = v.num_keys; i > pos; i--) {
        SetKeyAt(page->data, i, KeyAt(page->data, i - 1));
        SetLeafRidAt(page->data, i, LeafRidAt(page->data, i - 1));
    }
    SetKeyAt(page->data, pos, key);
    SetLeafRidAt(page->data, pos, rid);
    v.num_keys++;
    WriteHeader(page->data, v);
    buffer_->UnpinPage(table_id_, leaf, true);
}

auto DiskBPlusTreeIndex::Insert(const IndexKey &key, const RID &rid) -> void {
    page_id_t leaf = FindLeafPage(key);
    Page *page = buffer_->FetchPage(table_id_, leaf);
    NodeView v{};
    ReadHeader(page->data, v, leaf);
    const uint16_t cap = LeafCapacity();
    buffer_->UnpinPage(table_id_, leaf, false);

    if (v.num_keys < cap) {
        InsertIntoLeaf(leaf, key, rid);
        return;
    }
    // 叶子满：先插入再分裂（用临时溢出处理：先插到能放下，这里直接 split 后插）。
    InsertIntoLeaf(leaf, key, rid); // 临时超出 1 个，slot 容量按 cap 预留了 +1 余量？
    SplitLeaf(leaf);
}

// 叶子分裂：后半搬到新叶子，新分隔键 = 新叶子首键，上推父节点。
auto DiskBPlusTreeIndex::SplitLeaf(page_id_t leaf) -> void {
    Page *page = buffer_->FetchPage(table_id_, leaf);
    NodeView v{};
    ReadHeader(page->data, v, leaf);
    const uint16_t total = v.num_keys;
    const uint16_t mid = total / 2;

    page_id_t sib = AllocNode(true);
    Page *sib_page = buffer_->FetchPage(table_id_, sib);
    NodeView sv{};
    ReadHeader(sib_page->data, sv, sib);

    uint16_t j = 0;
    for (uint16_t i = mid; i < total; i++, j++) {
        SetKeyAt(sib_page->data, j, KeyAt(page->data, i));
        SetLeafRidAt(sib_page->data, j, LeafRidAt(page->data, i));
    }
    sv.num_keys = j;
    sv.next_leaf = v.next_leaf;
    v.num_keys = mid;
    v.next_leaf = sib;
    WriteHeader(page->data, v);
    WriteHeader(sib_page->data, sv);

    IndexKey up = KeyAt(sib_page->data, 0);
    buffer_->UnpinPage(table_id_, sib, true);
    buffer_->UnpinPage(table_id_, leaf, true);

    InsertIntoParent(leaf, up, sib);
}

// 找 target 在树中的父节点页号（沿 hint 键下行）。target==root 返回 -1。
auto DiskBPlusTreeIndex::FindParent(page_id_t root, page_id_t target, const IndexKey &hint) const -> page_id_t {
    if (root == target) return -1;
    page_id_t cur = root;
    page_id_t parent = -1;
    while (true) {
        Page *page = buffer_->FetchPage(table_id_, cur);
        NodeView v{};
        ReadHeader(page->data, v, cur);
        if (v.is_leaf) { buffer_->UnpinPage(table_id_, cur, false); break; }
        uint16_t idx = 0;
        while (idx < v.num_keys && !(hint < KeyAt(page->data, idx))) idx++;
        page_id_t child = ChildAt(page->data, idx);
        buffer_->UnpinPage(table_id_, cur, false);
        if (child == target) { parent = cur; break; }
        cur = child;
    }
    return parent;
}

// 把分隔键 + 右孩子插入父节点；root 分裂时保持根页号不变（抬升树高）。
auto DiskBPlusTreeIndex::InsertIntoParent(page_id_t left, const IndexKey &key, page_id_t right) -> void {
    if (left == root_page_id_) {
        // 稳定根：把当前根（left，已是分裂后的左半）整页内容搬到新页 new_left，
        // 再把根页本身改写成内部节点 [new_left, right]，根页号保持不变（便于持久化）。
        page_id_t new_left = AllocNode(true); // is_leaf 占位，下面整页覆盖
        Page *lp = buffer_->FetchPage(table_id_, left);
        Page *nlp = buffer_->FetchPage(table_id_, new_left);
        std::memcpy(nlp->data, lp->data, PAGE_SIZE);
        // 修正 new_left 的页号头（page_no 不存于页内，无需改；保持其余原样）。
        buffer_->UnpinPage(table_id_, new_left, true);

        // 把 root 页重写为内部节点。
        std::memset(lp->data, 0, PAGE_SIZE);
        NodeView rv{left, /*is_leaf=*/false, 1, -1};
        WriteHeader(lp->data, rv);
        SetKeyAt(lp->data, 0, key);
        SetChildAt(lp->data, 0, new_left);
        SetChildAt(lp->data, 1, right);
        buffer_->UnpinPage(table_id_, left, true);
        return; // root_page_id_ 不变
    }
    page_id_t parent = FindParent(root_page_id_, left, key);
    Page *pp = buffer_->FetchPage(table_id_, parent);
    NodeView pv{};
    ReadHeader(pp->data, pv, parent);
    // 找 left 在 parent 中的孩子下标。
    uint16_t ci = 0;
    while (ci <= pv.num_keys && ChildAt(pp->data, ci) != left) ci++;
    // 在位置 ci 插入分隔键，child(ci+1)=right。
    for (uint16_t i = pv.num_keys; i > ci; i--) {
        SetKeyAt(pp->data, i, KeyAt(pp->data, i - 1));
    }
    for (uint16_t i = pv.num_keys + 1; i > ci + 1; i--) {
        SetChildAt(pp->data, i, ChildAt(pp->data, i - 1));
    }
    SetKeyAt(pp->data, ci, key);
    SetChildAt(pp->data, ci + 1, right);
    pv.num_keys++;
    WriteHeader(pp->data, pv);
    const uint16_t cap = InternalCapacity();
    const bool overflow = pv.num_keys > cap;
    buffer_->UnpinPage(table_id_, parent, true);

    if (overflow) {
        SplitInternal(parent);
    }
}

// 内部节点分裂：中键上推，右半（含其孩子）搬到新节点。
auto DiskBPlusTreeIndex::SplitInternal(page_id_t node) -> void {
    Page *page = buffer_->FetchPage(table_id_, node);
    NodeView v{};
    ReadHeader(page->data, v, node);
    const uint16_t total = v.num_keys;
    const uint16_t mid = total / 2;
    IndexKey up = KeyAt(page->data, mid);

    page_id_t sib = AllocNode(false);
    Page *sib_page = buffer_->FetchPage(table_id_, sib);
    NodeView sv{};
    ReadHeader(sib_page->data, sv, sib);

    uint16_t j = 0;
    for (uint16_t i = mid + 1; i < total; i++, j++) {
        SetKeyAt(sib_page->data, j, KeyAt(page->data, i));
    }
    uint16_t cj = 0;
    for (uint16_t i = mid + 1; i <= total; i++, cj++) {
        SetChildAt(sib_page->data, cj, ChildAt(page->data, i));
    }
    sv.num_keys = j;
    v.num_keys = mid;
    WriteHeader(page->data, v);
    WriteHeader(sib_page->data, sv);
    buffer_->UnpinPage(table_id_, sib, true);
    buffer_->UnpinPage(table_id_, node, true);

    InsertIntoParent(node, up, sib);
}

auto DiskBPlusTreeIndex::Erase(const IndexKey &key, const RID &rid) -> bool {
    // 简化：仅在叶子内删除该 (key,rid)，不做下溢合并（不影响正确性，仅空间）。
    page_id_t leaf = FindLeafPage(key);
    bool erased = false;
    while (leaf >= 0) {
        Page *page = buffer_->FetchPage(table_id_, leaf);
        NodeView v{};
        ReadHeader(page->data, v, leaf);
        bool past = false;
        for (uint16_t i = 0; i < v.num_keys; i++) {
            IndexKey k = KeyAt(page->data, i);
            if (k < key) continue;
            if (key < k) { past = true; break; }
            if (LeafRidAt(page->data, i) == rid) {
                // 前移覆盖。
                for (uint16_t t = i; t + 1 < v.num_keys; t++) {
                    SetKeyAt(page->data, t, KeyAt(page->data, t + 1));
                    SetLeafRidAt(page->data, t, LeafRidAt(page->data, t + 1));
                }
                v.num_keys--;
                WriteHeader(page->data, v);
                erased = true;
                break;
            }
        }
        page_id_t next = v.next_leaf;
        buffer_->UnpinPage(table_id_, leaf, erased);
        if (erased || past) break;
        leaf = next;
    }
    return erased;
}
