//
// Created by huan.yang on 2026-05-07.
//
#pragma once
#include <memory>
#include <string>
#include <unordered_map>

#include "catalog/schema_version.h"
#include "catalog/table_catalog_entry.h"
#include "common/constants.h"
#include "common/types.h"
#include "parser/statment/create_table_statement.h"

namespace chickenDB {

class BufferManager;
class Page;

class Catalog {
public:
    // 纯内存模式，现有测试走这条路
    explicit Catalog() = default;

    // 磁盘持久化模式：构造时自动 LoadFromDisk（文件不存在则 InitFreshDisk）
    explicit Catalog(BufferManager *buffer_manager);

    ~Catalog() = default;

    auto CreateTable(const CreateTableStatement &statement, uint64_t create_ts = 0) -> TableCatalogEntry;
    auto DropTable(const std::string &table_name, uint64_t drop_ts = 0) -> bool;
    auto GetTable(const std::string &table_name) const -> const TableCatalogEntry *;
    auto GetTable(table_id_t table_id) const -> const TableCatalogEntry *;
    auto GetSchema(table_id_t table_id) const -> const SchemaPage *;
    auto TableExists(const std::string &table_name) const -> bool;

    // 磁盘生命周期（由带 BufferManager 的构造函数自动调用）
    auto LoadFromDisk() -> void;
    auto InitFreshDisk() -> void;

public:
    std::unordered_map<std::string, table_id_t> table_name_map_;
    std::unordered_map<table_id_t, TableCatalogEntry> table_entry_map_;
    std::unordered_map<table_id_t, std::unique_ptr<SchemaPage>> schema_map_;

private:
    auto AllocateTableId() -> table_id_t;
    // 纯内存模式返回顺序 ID（从 1 开始）；磁盘模式返回实际页号
    auto AllocateSchemaPageNo() -> page_id_t;
    auto BuildInitialSchema(const CreateTableStatement &statement, uint64_t create_ts)
        -> std::unique_ptr<SchemaPage>;

    // 磁盘辅助函数（buffer_manager_ == nullptr 时全部为 no-op）
    auto AllocateDiskPage() -> Page *;          // NewPage + 更新 next_free_page_no_
    auto PersistSchema(const SchemaPage &schema) -> page_id_t; // 写 schema page，返回页号
    auto PersistNewEntry(const TableCatalogEntry &entry) -> void;
    auto PersistUpdatedEntry(table_id_t table_id) -> void;
    auto PersistRootMeta() -> void;

    table_id_t next_table_id_{1};
    // 纯内存时：schema 顺序 ID（从 1 开始）；磁盘时：catalog 文件下一个空闲页号
    page_id_t next_free_page_no_{1};

    BufferManager *buffer_manager_{nullptr};
    // 当前正在追加的 TableCatalogPage 页号（磁盘模式）
    page_id_t current_catalog_page_no_{CATALOG_FIRST_TABLE_PAGE_NO};
    // table_id → 存储它的 catalog 页号（用于 DropTable 定位更新位置）
    std::unordered_map<table_id_t, page_id_t> entry_page_map_;
};

}
