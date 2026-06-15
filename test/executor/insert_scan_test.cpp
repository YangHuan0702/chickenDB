//
// Created by huan.yang on 2026-06-11.
//
// 端到端：CREATE TABLE -> 多条 INSERT(列式+zstd 写页) -> TableScanIterator 顺序扫描
// 解压回填 Chunk，校验定长列(NUMBER/DOUBLE)数据往返一致。
//
#include <cstdlib>
#include <filesystem>

#include "gtest/gtest.h"

#include "binder/binder.h"
#include "buffer/table_scan_iterator.h"
#include "executor/execution.h"
#include "parser/parser.h"
#include "planner/planner.h"

using namespace chickenDB;

namespace {
    // 把一条 SQL 走完 parser -> binder -> planner -> executor。
    auto RunSql(const std::string &sql, std::shared_ptr<BufferManager> &buffer,
                std::shared_ptr<Catalog> &catalog) -> void {        Parser parser;
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

TEST(InsertScan, InsertThenScanFixedLengthColumns) {
    // 用独立、干净的数据目录，避免上轮残留影响 table_id 分配。
    const std::string data_dir = "./data/insert_scan_test";
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

    // 解析器目前只支持单行 INSERT，因此分三条；正好产生 3 个数据页，顺带验证多页扫描。
    RunSql("insert into t (a, b) values (1, 1.5)", buffer, catalog);
    RunSql("insert into t (a, b) values (2, 2.5)", buffer, catalog);
    RunSql("insert into t (a, b) values (3, 3.5)", buffer, catalog);

    const auto *entry = catalog->GetTable(std::string("t"));
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->row_count, 3U);

    const table_id_t table_id = entry->table_id;
    const SchemaPage *schema = catalog->GetSchema(table_id);
    ASSERT_NE(schema, nullptr);
    ASSERT_EQ(schema->columns_.size(), 2U);

    const page_id_t page_count = buffer->GetPageCount(table_id);
    ASSERT_EQ(page_count, 3);

    TableScanIterator it(table_id, /*first=*/0, /*last=*/page_count - 1, buffer, const_cast<SchemaPage *>(schema));

    std::vector<int32_t> got_a;
    std::vector<double> got_b;
    Chunk chunk;
    while (it.Next(chunk)) {
        const size_t n = chunk.Count();
        const Vector &va = chunk.GetColumn(0);
        const Vector &vb = chunk.GetColumn(1);
        for (size_t r = 0; r < n; r++) {
            EXPECT_TRUE(va.IsValid(r));
            EXPECT_TRUE(vb.IsValid(r));
            got_a.push_back(va.GetValue<int32_t>(r));
            got_b.push_back(vb.GetValue<double>(r));
        }
    }

    ASSERT_EQ(got_a.size(), 3U);
    EXPECT_EQ(got_a[0], 1);
    EXPECT_EQ(got_a[1], 2);
    EXPECT_EQ(got_a[2], 3);
    EXPECT_DOUBLE_EQ(got_b[0], 1.5);
    EXPECT_DOUBLE_EQ(got_b[1], 2.5);
    EXPECT_DOUBLE_EQ(got_b[2], 3.5);

    std::filesystem::remove_all(data_dir, ec);
}
