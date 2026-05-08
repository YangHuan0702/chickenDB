//
// Created by huan.yang on 2026-05-07.
//
#pragma once
#include <memory>
#include <string>
#include <unordered_map>

#include "catalog/schema_version.h"
#include "catalog/table_catalog_entry.h"
#include "common/types.h"
#include "parser/statment/create_table_statement.h"

namespace chickenDB {
    class Catalog {
    public:
        explicit Catalog() = default;

        ~Catalog() = default;

        auto CreateTable(const CreateTableStatement &statement, uint64_t create_ts = 0) -> TableCatalogEntry;
        auto DropTable(const std::string &table_name, uint64_t drop_ts = 0) -> bool;
        auto GetTable(const std::string &table_name) const -> const TableCatalogEntry *;
        auto GetTable(obj_id_t table_id) const -> const TableCatalogEntry *;
        auto GetSchema(obj_id_t table_id) const -> const SchemaPage *;
        auto TableExists(const std::string &table_name) const -> bool;

    private:
        auto AllocateTableId() -> obj_id_t;
        auto AllocateSchemaPageId() -> uint32_t;
        auto BuildInitialSchema(const CreateTableStatement &statement, uint64_t create_ts) -> std::unique_ptr<SchemaPage>;

    public:
        std::unordered_map<std::string, obj_id_t> table_name_map_;
        std::unordered_map<obj_id_t, TableCatalogEntry> table_entry_map_;
        std::unordered_map<obj_id_t, std::unique_ptr<SchemaPage>> schema_map_;

    private:
        obj_id_t next_table_id_{1};
        uint32_t next_schema_page_id_{1};
    };
}
