//
// Created by huan.yang on 2026-06-11.
//
// 端到端：CREATE TABLE -> INSERT -> CREATE INDEX（走 parser->binder->planner->executor），
// 然后通过 catalog 取到索引、点查验证 RID，再用 TableHeap 回表验证取到的行值正确。
//
#include <cstdlib>
#include <filesystem>

#include "gtest/gtest.h"

#include "binder/binder.h"
#include "buffer/table_heap.h"
#include "catalog/index_catalog_entry.h"
#include "executor/execution.h"
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
}

TEST(CreateIndex, BuildAndLookup) {
    const std::string data_dir = "./data/create_index_test";
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

    RunSql("create table t (a INT, b DOUBLE)", buffer, catalog);
    RunSql("insert into t (a, b) values (10, 1.5)", buffer, catalog);
    RunSql("insert into t (a, b) values (20, 2.5)", buffer, catalog);
    RunSql("insert into t (a, b) values (30, 3.5)", buffer, catalog);

    RunSql("create index idx_a on t (a)", buffer, catalog);

    const IndexInfo *info = catalog->GetIndex("idx_a");
    ASSERT_NE(info, nullptr);
    ASSERT_NE(info->index, nullptr);

    // 点查 key=20 -> 应得到 1 个 RID。
    auto rids = info->index->Find(IndexKey(std::vector<double>{20}));
    ASSERT_EQ(rids.size(), 1U);

    // 回表验证该 RID 的行：a=20, b=2.5。
    const SchemaPage *schema = catalog->GetSchema(info->table_id);
    TableHeap heap(info->table_id, buffer, schema);
    Chunk row;
    std::vector<ColumnType> types;
    for (auto &c : schema->columns_) types.push_back(c.data_type);
    row.Init(types, 1);
    ASSERT_TRUE(heap.FetchRow(rids[0], row, 0));
    EXPECT_EQ(row.GetColumn(0).GetValue<int32_t>(0), 20);
    EXPECT_DOUBLE_EQ(row.GetColumn(1).GetValue<double>(0), 2.5);

    // 不存在的 key。
    EXPECT_TRUE(info->index->Find(IndexKey(std::vector<double>{999})).empty());

    std::filesystem::remove_all(data_dir, ec);
}
