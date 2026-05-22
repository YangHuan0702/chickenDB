#include "catalog/catalog.h"

#include <gtest/gtest.h>

#include "common/chicken_execption.h"

namespace chickenDB {
    TEST(CatalogTest, CreateAndLookupTable) {
        Catalog catalog;
        std::string table_name = "users";
        CreateTableStatement statement("users");
        statement.AddColumn({"id", ColumnType::NUMBER, 0});
        statement.AddColumn({"name", ColumnType::VARCHAR, 64});

        const auto entry = catalog.CreateTable(table_name,statement.columns_, 100);

        EXPECT_EQ(entry.table_id, 1);
        EXPECT_EQ(entry.GetTableName(), "users");
        EXPECT_EQ(entry.schema_page_id, 1);
        EXPECT_TRUE(catalog.TableExists("users"));

        const auto *table = catalog.GetTable("users");
        ASSERT_NE(table, nullptr);
        EXPECT_EQ(table->table_id, entry.table_id);

        const auto *schema = catalog.GetSchema(entry.table_id);
        ASSERT_NE(schema, nullptr);
        EXPECT_EQ(schema->version_.version, 1);
        EXPECT_EQ(schema->version_.effective_ts, 100);
        ASSERT_EQ(schema->columns_.size(), 2);
        EXPECT_EQ(schema->columns_[0].col_id, 1);
        EXPECT_EQ(schema->columns_[0].GetColumnName(), "id");
        EXPECT_EQ(schema->columns_[1].col_id, 2);
        EXPECT_EQ(schema->columns_[1].GetColumnName(), "name");
        EXPECT_EQ(schema->columns_[1].type_param, 64);
    }

    TEST(CatalogTest, RejectDuplicateTableName) {
        Catalog catalog;
        CreateTableStatement statement("users");
        statement.AddColumn({"id", ColumnType::NUMBER, 0});

        catalog.CreateTable(statement.table_name_,statement.columns_,0);

        EXPECT_THROW(catalog.CreateTable(statement.table_name_,statement.columns_,0), ChickenException);
    }

    TEST(CatalogTest, DropTableRemovesNameLookupAndMarksEntryDropped) {
        Catalog catalog;
        CreateTableStatement statement("users");
        statement.AddColumn({"id", ColumnType::NUMBER, 0});

        const auto entry = catalog.CreateTable(statement.table_name_,statement.columns_,0);

        EXPECT_TRUE(catalog.DropTable("users", 200));
        EXPECT_FALSE(catalog.TableExists("users"));
        EXPECT_EQ(catalog.GetTable("users"), nullptr);
        EXPECT_EQ(catalog.GetTable(entry.table_id), nullptr);

        const auto raw_entry = catalog.table_entry_map_.find(entry.table_id);
        ASSERT_NE(raw_entry, catalog.table_entry_map_.end());
        EXPECT_EQ(raw_entry->second.status, TableCatalogEntryStatus::DROPPED);
        EXPECT_EQ(raw_entry->second.drop_ts, 200);
    }
}
