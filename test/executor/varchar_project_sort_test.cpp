//
// 阶段4：VARCHAR 经 Project / ORDER BY / DISTINCT / GROUP BY 端到端。
//
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>

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
    auto AsStr(const Value &v) -> std::string {
        return std::holds_alternative<std::string>(v.value_) ? std::get<std::string>(v.value_)
                                                             : std::string();
    }
}

TEST(VarcharProjectSort, ProjectAndOrderBy) {
    const std::string dir = "./data/varchar_project_sort_test";
    Db db = MakeDb(dir);
    Session s(db.buffer, db.catalog, db.txn_mgr, db.vstore, db.log);
    s.Execute("create table t (id INT, name VARCHAR(32))");
    s.Execute("insert into t (id, name) values (1, 'charlie')");
    s.Execute("insert into t (id, name) values (2, 'alice')");
    s.Execute("insert into t (id, name) values (3, 'bob')");

    s.Execute("begin");

    // 仅投影 varchar 列
    s.Execute("select name from t");
    EXPECT_EQ(s.LastResult().size(), 3U);

    // ORDER BY varchar 升序
    s.Execute("select name from t order by name");
    ASSERT_EQ(s.LastResult().size(), 3U);
    EXPECT_EQ(AsStr(s.LastResult()[0][0]), "alice");
    EXPECT_EQ(AsStr(s.LastResult()[1][0]), "bob");
    EXPECT_EQ(AsStr(s.LastResult()[2][0]), "charlie");

    s.Execute("commit");

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

TEST(VarcharProjectSort, GroupByVarchar) {
    const std::string dir = "./data/varchar_groupby_test";
    Db db = MakeDb(dir);
    Session s(db.buffer, db.catalog, db.txn_mgr, db.vstore, db.log);
    s.Execute("create table t (city VARCHAR(16), v INT)");
    s.Execute("insert into t (city, v) values ('bj', 10)");
    s.Execute("insert into t (city, v) values ('bj', 20)");
    s.Execute("insert into t (city, v) values ('sh', 5)");

    s.Execute("begin");
    s.Execute("select city, sum(v) from t group by city");
    // 两组 bj/sh，每组一行。
    EXPECT_EQ(s.LastResult().size(), 2U);
    s.Execute("commit");

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

TEST(VarcharProjectSort, DistinctVarchar) {
    const std::string dir = "./data/varchar_distinct_test";
    Db db = MakeDb(dir);
    Session s(db.buffer, db.catalog, db.txn_mgr, db.vstore, db.log);
    s.Execute("create table t (id INT, name VARCHAR(16))");
    s.Execute("insert into t (id, name) values (1, 'x')");
    s.Execute("insert into t (id, name) values (2, 'x')");
    s.Execute("insert into t (id, name) values (3, 'y')");

    s.Execute("begin");
    s.Execute("select distinct name from t");
    EXPECT_EQ(s.LastResult().size(), 2U); // x, y
    s.Execute("commit");

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}
