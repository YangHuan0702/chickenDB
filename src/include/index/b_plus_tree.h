//
// Created by huan.yang on 2026-06-11.
//
#pragma once
#include <memory>
#include <vector>

#include "common/rid.h"
#include "index/index.h"
#include "index/index_key.h"

namespace chickenDB {
    // B+树索引：Index 接口的有序索引实现（key=IndexKey, value=RID，支持非唯一键）。
    // 这是内存版地基（可独立单测）；磁盘持久化版本在其之上构建。
    //
    // 结构：内部节点存 keys_ 和 children_（children = keys+1）；叶子节点存 keys_ 和
    // 与之并列的 rids_（每个 key 一组 RID），叶子用 next_ 串成链表以支持范围扫描。
    class BPlusTreeIndex : public Index {
    public:
        explicit BPlusTreeIndex(size_t order = 64);
        ~BPlusTreeIndex() override = default;

        auto Type() const -> IndexType override;
        auto SupportsRange() const -> bool override;

        auto Insert(const IndexKey &key, const RID &rid) -> void override;
        auto Erase(const IndexKey &key, const RID &rid) -> bool override;
        auto Find(const IndexKey &key) const -> std::vector<RID> override;
        auto Range(const IndexKey &lo, const IndexKey &hi) const -> std::vector<RID> override;
        auto ScanAll() const -> std::vector<std::pair<IndexKey, RID>> override;

    private:
        struct Node {
            bool is_leaf_;
            std::vector<IndexKey> keys_;
            std::vector<std::unique_ptr<Node>> children_; // 内部节点用
            std::vector<std::vector<RID>> rids_;          // 叶子节点用
            Node *next_{nullptr};                         // 叶子链表
            Node *parent_{nullptr};

            explicit Node(bool leaf) : is_leaf_(leaf) {}
        };

        auto FindLeaf(const IndexKey &key) const -> Node *;
        auto SplitLeaf(Node *leaf) -> void;
        auto SplitInternal(Node *node) -> void;
        auto InsertIntoParent(Node *left, const IndexKey &key, std::unique_ptr<Node> right) -> void;

        std::unique_ptr<Node> root_;
        size_t order_;
    };
}
