//
// Created by huan.yang on 2026-06-11.
//
// JOIN 从 SQL 端到端：两表 inner equi-join，经 Session 执行，验证连接结果。
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

TEST(JoinSql, TwoTableInnerEquiJoin) {
    const std::string dir = "./data/join_sql_test";
    Db db = MakeDb(dir);
    Session s(db.buffer, db.catalog, db.txn_mgr, db.vstore, db.log);

    // 两张表：emp(id, dept_id), dept(did, score)。
    s.Execute("create table emp (id INT, dept_id INT)");
    s.Execute("create table dept (did INT, score DOUBLE)");

    s.Execute("insert into emp (id, dept_id) values (1, 10)");
    s.Execute("insert into emp (id, dept_id) values (2, 20)");
    s.Execute("insert into emp (id, dept_id) values (3, 10)");

    s.Execute("insert into dept (did, score) values (10, 1.5)");
    s.Execute("insert into dept (did, score) values (20, 2.5)");

    // emp JOIN dept ON emp.dept_id = dept.did：
    //  emp(1,10)+dept(10) , emp(3,10)+dept(10) , emp(2,20)+dept(20) = 3 行。
    s.Execute("begin");
    s.Execute("select emp.id, dept.score from emp join dept on emp.dept_id = dept.did");
    EXPECT_EQ(s.LastResult().size(), 3U);
    s.Execute("commit");

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}
