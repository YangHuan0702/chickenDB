//
// Created by huan.yang on 2026-06-11.
//
// DELETE / UPDATE 的 MVCC 语义 + planner 自动选索引，端到端通过 Session 验证。
//
#include <cstdlib>
#include <filesystem>
#include <memory>

#include "gtest/gtest.h"

#include "catalog/catalog.h"
#include "buffer/buffer_manager.h"
#include "executor/session.h"
#include "transaction/transaction_manager.h"
#include "transaction/version_store.h"
#include "transaction/log_manager.h"

using namespace chickenDB;

namespace {
    struct Db {
        std::shared_ptr<LRUTableManager> lru;
        std::shared_ptr<BufferManager> buffer;
        std::shared_ptr<Catalog> catalog;
        std::shared_ptr<TransactionManager> txn_mgr;
        std::shared_ptr<VersionStore> vstore;
        std::shared_ptr<LogManager> log;
    };
    auto MakeDb(const std::string &dir) -> Db {
#ifdef _WIN32
        _putenv_s("CHICKENDB_DATA_PATH", dir.c_str());
#else
        setenv("CHICKENDB_DATA_PATH", dir.c_str(), 1);
#endif
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
        Db db;
        db.lru = std::make_shared<LRUTableManager>();
        db.buffer = std::make_shared<BufferManager>(db.lru);
        db.catalog = std::make_shared<Catalog>(db.buffer);
        db.txn_mgr = std::make_shared<TransactionManager>();
        db.vstore = std::make_shared<VersionStore>();
        db.log = std::make_shared<LogManager>();
        return db;
    }
}

TEST(DeleteUpdate, DeleteHidesMatchingRows) {
    const std::string dir = "./data/delete_test";
    Db db = MakeDb(dir);
    Session s(db.buffer, db.catalog, db.txn_mgr, db.vstore, db.log);
    s.Execute("create table t (a INT, b DOUBLE)");
    s.Execute("insert into t (a, b) values (1, 1.5)");
    s.Execute("insert into t (a, b) values (2, 2.5)");
    s.Execute("insert into t (a, b) values (3, 3.5)");

    s.Execute("delete from t where a = 2");

    s.Execute("begin");
    s.Execute("select a, b from t");
    EXPECT_EQ(s.LastResult().size(), 2U); // 删掉 a=2，剩 2 行
    s.Execute("commit");

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

TEST(DeleteUpdate, UpdateReplacesRow) {
    const std::string dir = "./data/update_test";
    Db db = MakeDb(dir);
    Session s(db.buffer, db.catalog, db.txn_mgr, db.vstore, db.log);
    s.Execute("create table t (a INT, b DOUBLE)");
    s.Execute("insert into t (a, b) values (1, 1.5)");
    s.Execute("insert into t (a, b) values (2, 2.5)");

    s.Execute("update t set b = 9.9 where a = 1");

    s.Execute("begin");
    s.Execute("select a, b from t");
    // 仍是 2 行（删旧插新），其中 a=1 的 b 变 9.9。
    ASSERT_EQ(s.LastResult().size(), 2U);
    bool found = false;
    for (auto &row : s.LastResult()) {
        // a 列是 int -> Value 持 int；b 列 double。
        if (std::holds_alternative<int>(row[0].value_) && std::get<int>(row[0].value_) == 1) {
            found = true;
            EXPECT_DOUBLE_EQ(std::get<double>(row[1].value_), 9.9);
        }
    }
    EXPECT_TRUE(found);
    s.Execute("commit");

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

TEST(IndexSelect, AutoUsesIndexForEquality) {
    const std::string dir = "./data/index_select_test";
    Db db = MakeDb(dir);
    Session s(db.buffer, db.catalog, db.txn_mgr, db.vstore, db.log);
    s.Execute("create table t (a INT, b DOUBLE)");
    s.Execute("insert into t (a, b) values (10, 1.5)");
    s.Execute("insert into t (a, b) values (20, 2.5)");
    s.Execute("insert into t (a, b) values (30, 3.5)");
    s.Execute("create index idx_a on t (a)");

    // WHERE a = 20 应命中索引（planner 自动选 IndexScan），结果 1 行。
    s.Execute("begin");
    s.Execute("select a, b from t where a = 20");
    ASSERT_EQ(s.LastResult().size(), 1U);
    EXPECT_EQ(std::get<int>(s.LastResult()[0][0].value_), 20);
    s.Execute("commit");

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}
