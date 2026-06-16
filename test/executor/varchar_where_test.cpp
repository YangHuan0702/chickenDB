//
// 阶段3：VARCHAR 的 WHERE 过滤端到端（= / <> / < / > / LIKE / NOT LIKE）+ 数值 WHERE 回归。
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

    // 取结果集第 col 列为字符串的集合大小辅助。
    auto AsStr(const Value &v) -> std::string {
        return std::holds_alternative<std::string>(v.value_) ? std::get<std::string>(v.value_)
                                                             : std::string();
    }
}

TEST(VarcharWhere, EqualityAndComparison) {
    const std::string dir = "./data/varchar_where_test";
    Db db = MakeDb(dir);
    Session s(db.buffer, db.catalog, db.txn_mgr, db.vstore, db.log);
    s.Execute("create table t (id INT, name VARCHAR(32))");
    s.Execute("insert into t (id, name) values (1, 'alice')");
    s.Execute("insert into t (id, name) values (2, 'bob')");
    s.Execute("insert into t (id, name) values (3, 'carol')");

    s.Execute("begin");

    // 等值
    s.Execute("select id, name from t where name = 'bob'");
    ASSERT_EQ(s.LastResult().size(), 1U);
    EXPECT_EQ(AsStr(s.LastResult()[0][1]), "bob");

    // 不等
    s.Execute("select id, name from t where name <> 'bob'");
    EXPECT_EQ(s.LastResult().size(), 2U);

    // 字典序 <
    s.Execute("select id, name from t where name < 'bob'");
    ASSERT_EQ(s.LastResult().size(), 1U);
    EXPECT_EQ(AsStr(s.LastResult()[0][1]), "alice");

    // 字典序 >
    s.Execute("select id, name from t where name > 'bob'");
    ASSERT_EQ(s.LastResult().size(), 1U);
    EXPECT_EQ(AsStr(s.LastResult()[0][1]), "carol");

    s.Execute("commit");

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

TEST(VarcharWhere, LikePatterns) {
    const std::string dir = "./data/varchar_like_test";
    Db db = MakeDb(dir);
    Session s(db.buffer, db.catalog, db.txn_mgr, db.vstore, db.log);
    s.Execute("create table t (id INT, name VARCHAR(32))");
    s.Execute("insert into t (id, name) values (1, 'alice')");
    s.Execute("insert into t (id, name) values (2, 'albert')");
    s.Execute("insert into t (id, name) values (3, 'bob')");

    s.Execute("begin");

    s.Execute("select id, name from t where name like 'al%'");
    EXPECT_EQ(s.LastResult().size(), 2U);

    s.Execute("select id, name from t where name like '%b%'");
    EXPECT_EQ(s.LastResult().size(), 2U); // 'albert','bob' 含 b；'alice' 不含

    s.Execute("select id, name from t where name like 'a_ice'");
    ASSERT_EQ(s.LastResult().size(), 1U);
    EXPECT_EQ(AsStr(s.LastResult()[0][1]), "alice");

    s.Execute("select id, name from t where name not like 'al%'");
    ASSERT_EQ(s.LastResult().size(), 1U);
    EXPECT_EQ(AsStr(s.LastResult()[0][1]), "bob");

    s.Execute("commit");

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

TEST(VarcharWhere, NumericWhereRegression) {
    // 回归：数值列 WHERE 不受字符串分流影响。
    const std::string dir = "./data/varchar_numregr_test";
    Db db = MakeDb(dir);
    Session s(db.buffer, db.catalog, db.txn_mgr, db.vstore, db.log);
    s.Execute("create table t (id INT, name VARCHAR(16))");
    s.Execute("insert into t (id, name) values (1, 'a')");
    s.Execute("insert into t (id, name) values (2, 'b')");
    s.Execute("insert into t (id, name) values (3, 'c')");

    s.Execute("begin");
    s.Execute("select id, name from t where id > 1");
    EXPECT_EQ(s.LastResult().size(), 2U);
    s.Execute("select id, name from t where id = 2");
    ASSERT_EQ(s.LastResult().size(), 1U);
    EXPECT_EQ(AsStr(s.LastResult()[0][1]), "b");
    s.Execute("commit");

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}
