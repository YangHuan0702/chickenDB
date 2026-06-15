//
// Created by huan.yang on 2026-06-11.
//
// version store 可见性单测（纯逻辑，不涉及磁盘）。验证快照隔离规则。
//
#include "gtest/gtest.h"

#include "transaction/transaction.h"
#include "transaction/version_store.h"

using namespace chickenDB;

TEST(VersionStore, OwnUncommittedInsertVisibleToSelf) {
    VersionStore vs;
    Transaction txn(/*id=*/1, /*read_ts=*/0);
    RID rid(0, 0);
    vs.OnInsert(rid, txn.GetTxnId());
    // 自己未提交的插入，对自己可见。
    EXPECT_TRUE(vs.IsVisible(rid, txn));
}

TEST(VersionStore, OthersUncommittedInsertInvisible) {
    VersionStore vs;
    RID rid(0, 0);
    vs.OnInsert(rid, /*txn=*/1); // 事务1 插入，未提交
    Transaction reader(/*id=*/2, /*read_ts=*/0);
    EXPECT_FALSE(vs.IsVisible(rid, reader));
}

TEST(VersionStore, CommittedInsertVisibleToLaterSnapshot) {
    VersionStore vs;
    RID rid(0, 0);
    vs.OnInsert(rid, 1);
    vs.CommitInsert(rid, /*commit_ts=*/5);
    // 快照 read_ts=10 >= 5 -> 可见。
    Transaction reader(2, 10);
    EXPECT_TRUE(vs.IsVisible(rid, reader));
    // 快照 read_ts=3 < 5 -> 不可见。
    Transaction old_reader(3, 3);
    EXPECT_FALSE(vs.IsVisible(rid, old_reader));
}

TEST(VersionStore, CommittedDeleteHidesRow) {
    VersionStore vs;
    RID rid(0, 0);
    vs.OnInsert(rid, 1);
    vs.CommitInsert(rid, 5);
    vs.OnDelete(rid, 6);
    vs.CommitDelete(rid, 8);

    // read_ts=10 在删除之后 -> 不可见。
    Transaction r1(2, 10);
    EXPECT_FALSE(vs.IsVisible(rid, r1));
    // read_ts=7 在删除提交(8)之前 -> 仍可见。
    Transaction r2(3, 7);
    EXPECT_TRUE(vs.IsVisible(rid, r2));
}

TEST(VersionStore, AbortInsertRemovesRow) {
    VersionStore vs;
    RID rid(0, 0);
    vs.OnInsert(rid, 1);
    vs.AbortInsert(rid);
    Transaction reader(2, 10);
    // 回滚插入的物理行仍可能留在页里，version store 必须显式隐藏它。
    EXPECT_FALSE(vs.IsVisible(rid, reader));
}

TEST(VersionStore, NoMetaDefaultsVisible) {
    VersionStore vs;
    RID rid(9, 9); // 从未登记（旧数据）
    Transaction reader(2, 10);
    EXPECT_TRUE(vs.IsVisible(rid, reader));
}
