//
// Created by huan.yang on 2026-06-11.
//
// 端到端 MVCC：通过 Session 执行 SQL，验证快照隔离——读事务看不到并发写事务
// 未提交的插入，提交后新快照可见。
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

TEST(Mvcc, SnapshotIsolationAcrossSessions) {
    const std::string dir = "./data/mvcc_test";
    Db db = MakeDb(dir);

    // 建表用一个自动提交会话。
    Session setup(db.buffer, db.catalog, db.txn_mgr, db.vstore, db.log);
    setup.Execute("create table t (a INT, b DOUBLE)");
    setup.Execute("insert into t (a, b) values (1, 1.5)");

    // 会话 writer：显式事务里再插一行，但先不提交。
    Session writer(db.buffer, db.catalog, db.txn_mgr, db.vstore, db.log);
    writer.Execute("begin");
    writer.Execute("insert into t (a, b) values (2, 2.5)");

    // 会话 reader：此刻开快照读，应只看到已提交的第 1 行。
    Session reader(db.buffer, db.catalog, db.txn_mgr, db.vstore, db.log);
    reader.Execute("begin");
    reader.Execute("select a, b from t");
    EXPECT_EQ(reader.LastResult().size(), 1U);

    // writer 提交。
    writer.Execute("commit");

    // 同一个 reader 事务（旧快照）仍看不到新行。
    reader.Execute("select a, b from t");
    EXPECT_EQ(reader.LastResult().size(), 1U);
    reader.Execute("commit");

    // 新事务能看到两行。
    Session reader2(db.buffer, db.catalog, db.txn_mgr, db.vstore, db.log);
    reader2.Execute("begin");
    reader2.Execute("select a, b from t");
    EXPECT_EQ(reader2.LastResult().size(), 2U);
    reader2.Execute("commit");

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

TEST(Mvcc, RollbackDiscardsInsert) {
    const std::string dir = "./data/mvcc_rollback_test";
    Db db = MakeDb(dir);

    Session s(db.buffer, db.catalog, db.txn_mgr, db.vstore, db.log);
    s.Execute("create table t (a INT)");
    s.Execute("begin");
    s.Execute("insert into t (a) values (42)");
    s.Execute("rollback");

    // 回滚后新事务看不到该行。
    s.Execute("begin");
    s.Execute("select a from t");
    EXPECT_EQ(s.LastResult().size(), 0U);
    s.Execute("commit");

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}
