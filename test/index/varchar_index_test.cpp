//
// 阶段5：VARCHAR 列建索引（内存版 B+树/Hash）点查 + 数值索引回归。
//
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "binder/binder.h"
#include "catalog/index_catalog_entry.h"
#include "executor/execution.h"
#include "index/index_key.h"
#include "parser/parser.h"
#include "planner/planner.h"

using namespace chickenDB;

namespace {
    auto RunSql(const std::string &sql, std::shared_ptr<BufferManager> &buffer,
                std::shared_ptr<Catalog> &catalog) -> void {
        Parser parser;
        parser.ParserQuery(sql);
        Binder binder(catalog);
        auto bound = binder.BinderStatement(std::move(parser.statements_));
        Planner planner;
        planner.SetCatalog(catalog);
        for (auto &stmt : bound) {
            auto logical = planner.CreateLogicalPlanner(std::move(stmt));
            auto physical = planner.CreatePhysicalPlanner(std::move(logical));
            auto ctx = std::make_unique<ExecutorContext>(buffer, catalog);
            Execution exec(std::move(ctx));
            exec.Exec(std::move(physical));
        }
    }

    auto MakeStrKey(const std::string &s) -> IndexKey {
        return IndexKey(std::vector<IndexKeyVal>{IndexKeyVal(s)});
    }
}

TEST(VarcharIndex, BuildAndPointLookup) {
    const std::string data_dir = "./data/varchar_index_test";
#ifdef _WIN32
    _putenv_s("CHICKENDB_DATA_PATH", data_dir.c_str());
#else
    setenv("CHICKENDB_DATA_PATH", data_dir.c_str(), 1);
#endif
    std::error_code ec;
    std::filesystem::remove_all(data_dir, ec);

    auto lru = std::make_shared<LRUTableManager>();
    auto buffer = std::make_shared<BufferManager>(lru);
    auto catalog = std::make_shared<Catalog>(buffer);

    RunSql("create table t (id INT, name VARCHAR(32))", buffer, catalog);
    RunSql("insert into t (id, name) values (1, 'alice')", buffer, catalog);
    RunSql("insert into t (id, name) values (2, 'bob')", buffer, catalog);
    RunSql("insert into t (id, name) values (3, 'carol')", buffer, catalog);
    RunSql("create index idx_name on t (name)", buffer, catalog);

    const IndexInfo *info = catalog->GetIndex("idx_name");
    ASSERT_NE(info, nullptr);
    ASSERT_NE(info->index, nullptr);
    // 变长键索引应为内存版（root_page_id == -1，非磁盘 B+树）。
    EXPECT_EQ(info->root_page_id, -1);

    // 字符串点查。
    auto rids = info->index->Find(MakeStrKey("bob"));
    EXPECT_EQ(rids.size(), 1U);
    EXPECT_TRUE(info->index->Find(MakeStrKey("zzz")).empty());

    std::filesystem::remove_all(data_dir, ec);
}

TEST(VarcharIndex, HashIndexOnVarchar) {
    const std::string data_dir = "./data/varchar_hash_index_test";
#ifdef _WIN32
    _putenv_s("CHICKENDB_DATA_PATH", data_dir.c_str());
#else
    setenv("CHICKENDB_DATA_PATH", data_dir.c_str(), 1);
#endif
    std::error_code ec;
    std::filesystem::remove_all(data_dir, ec);

    auto lru = std::make_shared<LRUTableManager>();
    auto buffer = std::make_shared<BufferManager>(lru);
    auto catalog = std::make_shared<Catalog>(buffer);

    RunSql("create table t (id INT, city VARCHAR(16))", buffer, catalog);
    RunSql("insert into t (id, city) values (1, 'bj')", buffer, catalog);
    RunSql("insert into t (id, city) values (2, 'sh')", buffer, catalog);
    RunSql("insert into t (id, city) values (3, 'bj')", buffer, catalog);

    // SQL 前端不带 USING <type>，索引类型默认 BPlusTree；这里直接经 catalog 建
    // varchar 上的 Hash 索引，验证哈希点查在字符串键上工作（非唯一返回多 RID）。
    const auto *tbl = catalog->GetTable(std::string("t"));
    ASSERT_NE(tbl, nullptr);
    const SchemaPage *schema = catalog->GetSchema(tbl->table_id);
    col_id_t city_col = 0;
    for (const auto &c : schema->columns_) {
        if (c.GetColumnName() == "city") city_col = c.col_id;
    }
    catalog->CreateIndex("idx_city", tbl->table_id, {city_col}, IndexType::Hash, /*unique=*/false);

    const IndexInfo *info = catalog->GetIndex("idx_city");
    ASSERT_NE(info, nullptr);
    ASSERT_NE(info->index, nullptr);
    // bj 非唯一 -> 2 个 RID。
    auto rids = info->index->Find(MakeStrKey("bj"));
    EXPECT_EQ(rids.size(), 2U);
    EXPECT_EQ(info->index->Find(MakeStrKey("sh")).size(), 1U);

    std::filesystem::remove_all(data_dir, ec);
}

TEST(VarcharIndex, NumericIndexRegression) {
    const std::string data_dir = "./data/varchar_index_numregr_test";
#ifdef _WIN32
    _putenv_s("CHICKENDB_DATA_PATH", data_dir.c_str());
#else
    setenv("CHICKENDB_DATA_PATH", data_dir.c_str(), 1);
#endif
    std::error_code ec;
    std::filesystem::remove_all(data_dir, ec);

    auto lru = std::make_shared<LRUTableManager>();
    auto buffer = std::make_shared<BufferManager>(lru);
    auto catalog = std::make_shared<Catalog>(buffer);

    RunSql("create table t (a INT, b VARCHAR(8))", buffer, catalog);
    RunSql("insert into t (a, b) values (10, 'x')", buffer, catalog);
    RunSql("insert into t (a, b) values (20, 'y')", buffer, catalog);
    RunSql("create index idx_a on t (a)", buffer, catalog);

    const IndexInfo *info = catalog->GetIndex("idx_a");
    ASSERT_NE(info, nullptr);
    // 数值键索引应为磁盘版 B+树（root_page_id >= 0），变长列不影响数值索引。
    EXPECT_GE(info->root_page_id, 0);
    auto rids = info->index->Find(IndexKey(std::vector<double>{20}));
    EXPECT_EQ(rids.size(), 1U);

    std::filesystem::remove_all(data_dir, ec);
}
