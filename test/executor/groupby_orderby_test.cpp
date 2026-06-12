//
// Created by huan.yang on 2026-06-11.
//
// GROUP BY 聚合 与 ORDER BY 排序 从 SQL 端到端验证（经 Session）。
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

TEST(GroupBy, SumByGroup) {
    const std::string dir = "./data/groupby_test";
    Db db = MakeDb(dir);
    Session s(db.buffer, db.catalog, db.txn_mgr, db.vstore, db.log);
    s.Execute("create table t (g INT, v INT)");
    s.Execute("insert into t (g, v) values (1, 10)");
    s.Execute("insert into t (g, v) values (1, 20)");
    s.Execute("insert into t (g, v) values (2, 5)");

    s.Execute("begin");
    s.Execute("select g, sum(v) from t group by g");
    // 输出每组一行：[g, sum, count]。两组。
    ASSERT_EQ(s.LastResult().size(), 2U);
    s.Execute("commit");

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

TEST(OrderBy, SortsAscending) {
    const std::string dir = "./data/orderby_test";
    Db db = MakeDb(dir);
    Session s(db.buffer, db.catalog, db.txn_mgr, db.vstore, db.log);
    s.Execute("create table t (a INT, b INT)");
    s.Execute("insert into t (a, b) values (3, 30)");
    s.Execute("insert into t (a, b) values (1, 10)");
    s.Execute("insert into t (a, b) values (2, 20)");

    s.Execute("begin");
    s.Execute("select a, b from t order by a");
    ASSERT_EQ(s.LastResult().size(), 3U);
    EXPECT_EQ(std::get<int>(s.LastResult()[0][0].value_), 1);
    EXPECT_EQ(std::get<int>(s.LastResult()[1][0].value_), 2);
    EXPECT_EQ(std::get<int>(s.LastResult()[2][0].value_), 3);
    s.Execute("commit");

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

TEST(IndexMaintenance, InsertAfterCreateIndexIsFound) {
    const std::string dir = "./data/idx_maint_test";
    Db db = MakeDb(dir);
    Session s(db.buffer, db.catalog, db.txn_mgr, db.vstore, db.log);
    s.Execute("create table t (a INT, b DOUBLE)");
    s.Execute("insert into t (a, b) values (1, 1.5)");
    s.Execute("create index idx_a on t (a)");
    // 建索引后再插入：索引应被增量维护，能查到。
    s.Execute("insert into t (a, b) values (2, 2.5)");

    s.Execute("begin");
    s.Execute("select a, b from t where a = 2");
    ASSERT_EQ(s.LastResult().size(), 1U);
    EXPECT_EQ(std::get<int>(s.LastResult()[0][0].value_), 2);
    s.Execute("commit");

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

TEST(OrderBy, SortsDescending) {
    const std::string dir = "./data/orderby_desc_test";
    Db db = MakeDb(dir);
    Session s(db.buffer, db.catalog, db.txn_mgr, db.vstore, db.log);
    s.Execute("create table t (a INT, b INT)");
    s.Execute("insert into t (a, b) values (1, 10)");
    s.Execute("insert into t (a, b) values (3, 30)");
    s.Execute("insert into t (a, b) values (2, 20)");

    s.Execute("begin");
    s.Execute("select a, b from t order by a desc");
    ASSERT_EQ(s.LastResult().size(), 3U);
    EXPECT_EQ(std::get<int>(s.LastResult()[0][0].value_), 3);
    EXPECT_EQ(std::get<int>(s.LastResult()[1][0].value_), 2);
    EXPECT_EQ(std::get<int>(s.LastResult()[2][0].value_), 1);
    s.Execute("commit");

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

TEST(GroupBy, MaxByGroup) {
    const std::string dir = "./data/groupby_max_test";
    Db db = MakeDb(dir);
    Session s(db.buffer, db.catalog, db.txn_mgr, db.vstore, db.log);
    s.Execute("create table t (g INT, v INT)");
    s.Execute("insert into t (g, v) values (1, 10)");
    s.Execute("insert into t (g, v) values (1, 30)");
    s.Execute("insert into t (g, v) values (1, 20)");

    s.Execute("begin");
    s.Execute("select g, max(v) from t group by g");
    // 一组，[g=1, max=30]。
    ASSERT_EQ(s.LastResult().size(), 1U);
    ASSERT_EQ(s.LastResult()[0].size(), 2U);
    EXPECT_EQ(std::get<double>(s.LastResult()[0][1].value_), 30.0);
    s.Execute("commit");

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

