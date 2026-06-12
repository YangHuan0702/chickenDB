//
// Created by huan.yang on 2026-06-11.
//
#include <memory>
#include <vector>

#include "gtest/gtest.h"

#include "index/b_plus_tree.h"
#include "index/index.h"

using namespace chickenDB;

namespace {
    auto K(double v) -> IndexKey { return IndexKey(std::vector<double>{v}); }
}

TEST(BPlusTree, InsertAndPointLookup) {
    BPlusTreeIndex tree(/*order=*/4);
    for (int i = 0; i < 100; i++) {
        tree.Insert(K(i), RID(i, 0));
    }
    for (int i = 0; i < 100; i++) {
        auto rids = tree.Find(K(i));
        ASSERT_EQ(rids.size(), 1U) << "key " << i;
        EXPECT_EQ(rids[0].page_no, i);
    }
    EXPECT_TRUE(tree.Find(K(1000)).empty());
}

TEST(BPlusTree, NonUniqueKeys) {
    BPlusTreeIndex tree(4);
    tree.Insert(K(5), RID(1, 0));
    tree.Insert(K(5), RID(1, 1));
    tree.Insert(K(5), RID(2, 0));
    auto rids = tree.Find(K(5));
    EXPECT_EQ(rids.size(), 3U);
}

TEST(BPlusTree, RangeScanSorted) {
    BPlusTreeIndex tree(4);
    // 乱序插入。
    int order[] = {7, 1, 9, 3, 5, 2, 8, 4, 6, 0};
    for (int v : order) {
        tree.Insert(K(v), RID(v, 0));
    }
    auto rids = tree.Range(K(3), K(7));
    ASSERT_EQ(rids.size(), 5U);
    for (size_t i = 0; i < rids.size(); i++) {
        EXPECT_EQ(rids[i].page_no, static_cast<page_id_t>(3 + i));
    }
}

TEST(BPlusTree, Erase) {
    BPlusTreeIndex tree(4);
    for (int i = 0; i < 20; i++) tree.Insert(K(i), RID(i, 0));
    EXPECT_TRUE(tree.Erase(K(10), RID(10, 0)));
    EXPECT_TRUE(tree.Find(K(10)).empty());
    EXPECT_FALSE(tree.Erase(K(10), RID(10, 0)));
    // 其余键仍在。
    EXPECT_EQ(tree.Find(K(9)).size(), 1U);
    EXPECT_EQ(tree.Find(K(11)).size(), 1U);
}

TEST(BPlusTree, CompositeKey) {
    BPlusTreeIndex tree(4);
    tree.Insert(IndexKey(std::vector<double>{1, 2}), RID(1, 0));
    tree.Insert(IndexKey(std::vector<double>{1, 3}), RID(1, 1));
    tree.Insert(IndexKey(std::vector<double>{2, 1}), RID(2, 0));
    EXPECT_EQ(tree.Find(IndexKey(std::vector<double>{1, 2})).size(), 1U);
    auto r = tree.Range(IndexKey(std::vector<double>{1, 0}), IndexKey(std::vector<double>{1, 9}));
    EXPECT_EQ(r.size(), 2U); // (1,2) 和 (1,3)
}

// 通过抽象 Index 接口使用，验证多态分发与能力查询。
TEST(BPlusTree, ThroughIndexInterface) {
    std::unique_ptr<Index> idx = std::make_unique<BPlusTreeIndex>(4);
    EXPECT_EQ(idx->Type(), IndexType::BPlusTree);
    EXPECT_TRUE(idx->SupportsRange());
    idx->Insert(K(1), RID(1, 0));
    idx->Insert(K(2), RID(2, 0));
    EXPECT_EQ(idx->Find(K(1)).size(), 1U);
    EXPECT_EQ(idx->Range(K(1), K(2)).size(), 2U);
}

