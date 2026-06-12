//
// Created by huan.yang on 2026-06-11.
//
#pragma once
#include <memory>
#include <vector>

#include "buffer/buffer_manager.h"
#include "common/rid.h"
#include "index/index.h"
#include "index/index_key.h"

namespace chickenDB {
    // 磁盘版 B+树索引：节点即页（PageType::INDEX），通过 BufferManager 读写，重启后从
    // root_page_id 直接加载（无需扫表重建）。键为定长（n_key_cols 个 double），value=RID。
    //
    // 节点页布局（小端，紧凑）：
    //   [0]  magic(u32)=PAGE_MAGIC_NUM
    //   [4]  page_type(u8)=INDEX
    //   [5]  is_leaf(u8)
    //   [6]  num_keys(u16)
    //   [8]  next_leaf(int64)        仅叶子用，叶子链表指针（-1 结束）
    //   [16] keys 区：num_keys × (n_key_cols × 8B)
    //   叶子：紧跟 RID 区：num_keys × (page_no int64 + row_idx u32)
    //   内部：紧跟 child 区：(num_keys+1) × (page_id int64)
    //
    // 非唯一键：同键多 RID 存为多个相邻 slot（键重复）。Find 收集相等区间。
    class DiskBPlusTreeIndex : public Index {
    public:
        DiskBPlusTreeIndex(std::shared_ptr<BufferManager> buffer, table_id_t table_id,
                           size_t key_cols, page_id_t root_page_id);

        auto Type() const -> IndexType override { return IndexType::BPlusTree; }
        auto SupportsRange() const -> bool override { return true; }

        auto Insert(const IndexKey &key, const RID &rid) -> void override;
        auto Erase(const IndexKey &key, const RID &rid) -> bool override;
        auto Find(const IndexKey &key) const -> std::vector<RID> override;
        auto Range(const IndexKey &lo, const IndexKey &hi) const -> std::vector<RID> override;
        auto ScanAll() const -> std::vector<std::pair<IndexKey, RID>> override;

        // 索引的根页号（建索引后回填到 catalog 持久化）。
        auto RootPageId() const -> page_id_t { return root_page_id_; }

    private:
        // 节点页的内存视图（解析自 page->data）。
        struct NodeView {
            page_id_t page_no;
            bool is_leaf;
            uint16_t num_keys;
            page_id_t next_leaf;
        };

        auto ReadHeader(const char *data, NodeView &v, page_id_t page_no) const -> void;
        auto WriteHeader(char *data, const NodeView &v) const -> void;
        auto KeyAt(const char *data, uint16_t i) const -> IndexKey;
        auto SetKeyAt(char *data, uint16_t i, const IndexKey &k) const -> void;
        auto LeafRidAt(const char *data, uint16_t i) const -> RID;
        auto SetLeafRidAt(char *data, uint16_t i, const RID &rid) const -> void;
        auto ChildAt(const char *data, uint16_t i) const -> page_id_t;
        auto SetChildAt(char *data, uint16_t i, page_id_t child) const -> void;

        auto KeyBytes() const -> size_t { return key_cols_ * sizeof(double); }
        auto LeafSlots() const -> uint16_t;      // 物理 slot 数（含 1 个溢出预留）
        auto InternalSlots() const -> uint16_t;
        auto LeafCapacity() const -> uint16_t;     // 逻辑容量 = LeafSlots()-1
        auto InternalCapacity() const -> uint16_t; // 逻辑容量 = InternalSlots()-1

        auto AllocNode(bool is_leaf) -> page_id_t;
        auto FindLeafPage(const IndexKey &key) const -> page_id_t;
        auto InsertIntoLeaf(page_id_t leaf, const IndexKey &key, const RID &rid) -> void;
        auto SplitLeaf(page_id_t leaf) -> void;
        auto InsertIntoParent(page_id_t left, const IndexKey &key, page_id_t right) -> void;
        auto SplitInternal(page_id_t node) -> void;
        // 返回 node 在 parent 中的位置 + parent page（-1 表示 node 是 root）。
        auto FindParent(page_id_t root, page_id_t target, const IndexKey &hint) const -> page_id_t;

        std::shared_ptr<BufferManager> buffer_;
        table_id_t table_id_;
        size_t key_cols_;
        page_id_t root_page_id_;
    };
}
