//
// Created by huan.yang on 2026-06-11.
//
// 磁盘版 B+树：节点即页，验证插入/点查/范围/遍历，并跨“重开”从根页恢复。
//
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <vector>

#include "gtest/gtest.h"

#include "buffer/buffer_manager.h"
#include "disk/table_manager.h"
#include "index/disk_b_plus_tree.h"

using namespace chickenDB;

namespace {
    auto K(double v) -> IndexKey { return IndexKey(std::vector<double>{v}); }

    auto SetDir(const std::string &dir) -> void {
#ifdef _WIN32
        _putenv_s("CHICKENDB_DATA_PATH", dir.c_str());
#else
        setenv("CHICKENDB_DATA_PATH", dir.c_str(), 1);
#endif
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
    }
}

TEST(DiskBPlusTree, InsertFindRangeWithSplits) {
    SetDir("./data/disk_bpt_test");
    auto lru = std::make_shared<LRUTableManager>();
    auto buffer = std::make_shared<BufferManager>(lru);
    const table_id_t tid = 7;

    DiskBPlusTreeIndex tree(buffer, tid, /*key_cols=*/1, /*root=*/-1);
    // 插入足够多以触发多次分裂。
    const int N = 5000;
    for (int i = 0; i < N; i++) {
        tree.Insert(K(i), RID(i % 100, i));
    }
    // 点查。
    for (int i = 0; i < N; i += 137) {
        auto rids = tree.Find(K(i));
        ASSERT_EQ(rids.size(), 1U) << "key " << i;
        EXPECT_EQ(rids[0].row_idx, static_cast<uint32_t>(i));
    }
    // 范围 [1000, 1009] -> 10 个。
    auto r = tree.Range(K(1000), K(1009));
    EXPECT_EQ(r.size(), 10U);
    // 遍历总数。
    EXPECT_EQ(tree.ScanAll().size(), static_cast<size_t>(N));
    EXPECT_TRUE(tree.Find(K(999999)).empty());

    std::error_code ec;
    std::filesystem::remove_all("./data/disk_bpt_test", ec);
}

TEST(DiskBPlusTree, NonUniqueKeys) {
    SetDir("./data/disk_bpt_nonuniq");
    auto lru = std::make_shared<LRUTableManager>();
    auto buffer = std::make_shared<BufferManager>(lru);
    DiskBPlusTreeIndex tree(buffer, 8, 1, -1);
    for (int i = 0; i < 50; i++) tree.Insert(K(5), RID(0, i)); // 同键多 RID
    EXPECT_EQ(tree.Find(K(5)).size(), 50U);

    std::error_code ec;
    std::filesystem::remove_all("./data/disk_bpt_nonuniq", ec);
}

TEST(DiskBPlusTree, ReopenFromRootPersists) {
    SetDir("./data/disk_bpt_reopen");
    auto lru = std::make_shared<LRUTableManager>();
    auto buffer = std::make_shared<BufferManager>(lru);
    const table_id_t tid = 9;

    page_id_t root = -1;
    {
        DiskBPlusTreeIndex tree(buffer, tid, 1, -1);
        for (int i = 0; i < 2000; i++) tree.Insert(K(i), RID(0, i));
        root = tree.RootPageId();
    }
    // 用同一 buffer（数据已在 pool/磁盘），从已知根页重开，应能查到。
    DiskBPlusTreeIndex reopened(buffer, tid, 1, root);
    EXPECT_EQ(reopened.Find(K(1500)).size(), 1U);
    EXPECT_EQ(reopened.ScanAll().size(), 2000U);

    std::error_code ec;
    std::filesystem::remove_all("./data/disk_bpt_reopen", ec);
}
