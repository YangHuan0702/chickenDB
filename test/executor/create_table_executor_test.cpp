//
// Created by huan.yang on 2026-05-22.
//
#include "binder/binder.h"
#include "gtest/gtest.h"
#include "executor/execution.h"
#include "parser/parser.h"
#include "planner/planner.h"

using namespace chickenDB;

TEST(Executor,BasicCreateTableExecutorTest) {
    std::string sql = "create table users (id VARCHAR(20),name VARCHAR(50),age INT)";
    Parser parser;
    parser.ParserQuery(sql);

    auto lru_table_manager = std::make_shared<LRUTableManager>();
    auto buffer_manager = std::make_shared<BufferManager>(lru_table_manager);

    auto catalog = std::make_shared<Catalog>(buffer_manager);

    Binder binder(catalog);
    std::vector<std::unique_ptr<BoundStatement>> bound_statement = binder.BinderStatement(std::move(parser.statements_));

    auto executor_context = std::make_unique<ExecutorContext>(buffer_manager,catalog);

    Planner planner;
    Execution executor(std::move(executor_context));
    for (auto &statement : bound_statement) {
        auto logical_operator = planner.CreateLogicalPlanner(std::move(statement));

        auto physical_operator = planner.CreatePhysicalPlanner(std::move(logical_operator));

        executor.Exec(std::move(physical_operator));
    }

    std::string table_name = "users";
    auto table_catalog_entry = catalog->GetTable(table_name);
    ASSERT_NE(table_catalog_entry, nullptr);
}



