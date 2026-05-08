#include "catalog/catalog.h"

#include <utility>

#include "common/chicken_execption.h"

namespace chickenDB {
    auto Catalog::CreateTable(const CreateTableStatement &statement, uint64_t create_ts) -> TableCatalogEntry {
        ChickenException::AssertCondition(!statement.table_name_.empty(), "table name can not be empty");
        ChickenException::AssertCondition(!TableExists(statement.table_name_), "table already exists: " + statement.table_name_);
        ChickenException::AssertCondition(!statement.columns_.empty(), "table must contain at least one column");

        const auto table_id = AllocateTableId();
        const auto schema_page_id = AllocateSchemaPageId();

        auto schema = BuildInitialSchema(statement, create_ts);

        TableCatalogEntry entry;
        entry.table_id = table_id;
        entry.SetTableName(statement.table_name_);
        entry.schema_page_id = schema_page_id;
        entry.create_ts = create_ts;

        table_name_map_.emplace(statement.table_name_, table_id);
        table_entry_map_.emplace(table_id, entry);
        schema_map_.emplace(table_id, std::move(schema));

        return entry;
    }

    auto Catalog::DropTable(const std::string &table_name, uint64_t drop_ts) -> bool {
        const auto table_iter = table_name_map_.find(table_name);
        if (table_iter == table_name_map_.end()) {
            return false;
        }

        const auto entry_iter = table_entry_map_.find(table_iter->second);
        if (entry_iter == table_entry_map_.end()) {
            table_name_map_.erase(table_iter);
            return false;
        }

        entry_iter->second.status = TableCatalogEntryStatus::DROPPED;
        entry_iter->second.drop_ts = drop_ts;
        table_name_map_.erase(table_iter);
        return true;
    }

    auto Catalog::GetTable(const std::string &table_name) const -> const TableCatalogEntry * {
        const auto table_iter = table_name_map_.find(table_name);
        if (table_iter == table_name_map_.end()) {
            return nullptr;
        }
        return GetTable(table_iter->second);
    }

    auto Catalog::GetTable(obj_id_t table_id) const -> const TableCatalogEntry * {
        const auto entry_iter = table_entry_map_.find(table_id);
        if (entry_iter == table_entry_map_.end() || !entry_iter->second.IsActive()) {
            return nullptr;
        }
        return &entry_iter->second;
    }

    auto Catalog::GetSchema(obj_id_t table_id) const -> const SchemaPage * {
        const auto entry = GetTable(table_id);
        if (entry == nullptr) {
            return nullptr;
        }

        const auto schema_iter = schema_map_.find(table_id);
        if (schema_iter == schema_map_.end()) {
            return nullptr;
        }
        return schema_iter->second.get();
    }

    auto Catalog::TableExists(const std::string &table_name) const -> bool {
        return GetTable(table_name) != nullptr;
    }

    auto Catalog::AllocateTableId() -> obj_id_t {
        return next_table_id_++;
    }

    auto Catalog::AllocateSchemaPageId() -> uint32_t {
        return next_schema_page_id_++;
    }

    auto Catalog::BuildInitialSchema(const CreateTableStatement &statement, uint64_t create_ts) -> std::unique_ptr<SchemaPage> {
        auto schema = std::make_unique<SchemaPage>();
        schema->version_.version = 1;
        schema->version_.effective_ts = create_ts;

        col_id_t next_column_id = 1;
        for (const auto &column : statement.columns_) {
            ColDef col_def;
            col_def.col_id = next_column_id++;
            col_def.SetColumnName(column.name_);
            col_def.data_type = column.type_;
            col_def.type_param = static_cast<uint16_t>(column.size_);
            schema->AddColumn(col_def);
        }

        return schema;
    }
}
