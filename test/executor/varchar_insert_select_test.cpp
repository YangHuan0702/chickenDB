//
// 阶段2：端到端 VARCHAR。CREATE TABLE varchar(n) -> INSERT 字符串(含 NULL) ->
// TableScanIterator 扫描回读，校验变长列数据往返一致 + 多页扫描。
//
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "binder/binder.h"
#include "buffer/table_scan_iterator.h"
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

TEST(VarcharInsertScan, InsertThenScanVarcharColumn) {
    const std::string data_dir = "./data/varchar_insert_select_test";
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

    // 单行 INSERT，三条 -> 三页，顺带验证多页扫描。第三条 name 为 NULL。
    RunSql("insert into t (id, name) values (1, 'alice')", buffer, catalog);
    RunSql("insert into t (id, name) values (2, 'bob')", buffer, catalog);
    RunSql("insert into t (id, name) values (3, null)", buffer, catalog);

    const auto *entry = catalog->GetTable(std::string("t"));
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->row_count, 3U);

    const table_id_t table_id = entry->table_id;
    const SchemaPage *schema = catalog->GetSchema(table_id);
    ASSERT_NE(schema, nullptr);
    ASSERT_EQ(schema->columns_.size(), 2U);
    EXPECT_EQ(schema->columns_[1].data_type, ColumnType::VARCHAR);

    const page_id_t page_count = buffer->GetPageCount(table_id);

    TableScanIterator it(table_id, 0, page_count - 1, buffer, const_cast<SchemaPage *>(schema));

    std::vector<int32_t> ids;
    std::vector<std::string> names;
    std::vector<bool> name_valid;
    Chunk chunk;
    while (it.Next(chunk)) {
        const size_t n = chunk.Count();
        const Vector &vid = chunk.GetColumn(0);
        const Vector &vname = chunk.GetColumn(1);
        ASSERT_TRUE(vname.IsVar());
        for (size_t r = 0; r < n; r++) {
            ids.push_back(vid.GetValue<int32_t>(r));
            name_valid.push_back(vname.IsValid(r));
            names.push_back(vname.IsValid(r) ? std::string(vname.GetString(r)) : std::string());
        }
    }

    ASSERT_EQ(ids.size(), 3U);
    EXPECT_EQ(ids[0], 1);
    EXPECT_EQ(names[0], "alice");
    EXPECT_TRUE(name_valid[0]);
    EXPECT_EQ(ids[1], 2);
    EXPECT_EQ(names[1], "bob");
    EXPECT_TRUE(name_valid[1]);
    EXPECT_EQ(ids[2], 3);
    EXPECT_FALSE(name_valid[2]); // NULL

    std::filesystem::remove_all(data_dir, ec);
}
