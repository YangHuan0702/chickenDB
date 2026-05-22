//
// Created by huan.yang on 2026-05-07.
//
#pragma once
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>

#include "buffer/buffer_manager.h"
#include "catalog/schema_version.h"
#include "catalog/table_catalog_entry.h"
#include "common/constants.h"
#include "common/types.h"
#include "parser/statment/create_table_statement.h"

namespace chickenDB {


class Catalog {
public:
    explicit Catalog() = default;

    explicit Catalog(std::shared_ptr<BufferManager> buffer_manager);

    ~Catalog() = default;

    auto CreateTable(const std::string& table_name,const std::vector<ColumnDefine>& colums_, uint64_t create_ts = 0) -> TableCatalogEntry;
    auto DropTable(const std::string &table_name, uint64_t drop_ts = 0) -> bool;
    auto GetTable(const std::string &table_name) const -> const TableCatalogEntry *;
    auto GetTable(table_id_t table_id) const -> const TableCatalogEntry *;
    auto GetSchema(table_id_t table_id) const -> const SchemaPage *;
    auto TableExists(const std::string &table_name) const -> bool;

    auto LoadFromDisk() -> void;
    auto InitFreshDisk() -> void;

    std::unordered_map<std::string, table_id_t> table_name_map_;
    std::unordered_map<table_id_t, TableCatalogEntry> table_entry_map_;
    std::unordered_map<table_id_t, std::unique_ptr<SchemaPage>> schema_map_;

private:
    auto GetTableLocked(const std::string &table_name) const -> const TableCatalogEntry *;
    auto GetTableLocked(table_id_t table_id) const -> const TableCatalogEntry *;

    auto AllocateTableId() -> table_id_t;
    auto AllocateSchemaPageNo() -> page_id_t;
    auto BuildInitialSchema(const std::string& table_name,const std::vector<ColumnDefine>& colums_, uint64_t create_ts)
        -> std::unique_ptr<SchemaPage>;

    auto AllocateDiskPage() -> Page *;
    auto PersistSchema(const SchemaPage &schema) -> page_id_t;
    auto PersistNewEntry(const TableCatalogEntry &entry) -> void;
    auto PersistUpdatedEntry(table_id_t table_id) -> void;
    auto PersistRootMeta() -> void;

    table_id_t next_table_id_{1};
    page_id_t next_free_page_no_{1};

    std::shared_ptr<BufferManager> buffer_manager_{nullptr};
    page_id_t current_catalog_page_no_{CATALOG_FIRST_TABLE_PAGE_NO};
    std::unordered_map<table_id_t, page_id_t> entry_page_map_;

    mutable std::shared_mutex rw_mutex_;
};

}
