//
// Created by huan.yang on 2026-05-07.
//
#pragma once
#include <functional>
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>

#include "buffer/buffer_manager.h"
#include "catalog/schema_version.h"
#include "catalog/table_catalog_entry.h"
#include "catalog/index_catalog_entry.h"
#include "common/constants.h"
#include "common/rid.h"
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

    // 在已有行数基础上增加 delta 行，并把更新后的 entry 刷回磁盘。
    // 供 INSERT 执行器在写入数据页后调用。
    auto AddRowCount(table_id_t table_id, uint64_t delta) -> void;

    // ---- 索引注册（本阶段内存常驻；磁盘持久化后续）----
    // 创建并注册一个索引，返回其 index_id。不立即填充数据（由执行器扫表填充）。
    auto CreateIndex(const std::string &index_name, table_id_t table_id,
                     const std::vector<col_id_t> &key_cols, IndexType type,
                     bool unique) -> uint32_t;
    // 取某张表的全部索引（用于 planner 选择索引扫描）。
    auto GetTableIndexes(table_id_t table_id) const -> std::vector<const IndexInfo *>;
    // 按名取索引。
    auto GetIndex(const std::string &index_name) const -> const IndexInfo *;

    // DML 索引维护：对某表的全部活索引，按行的键列值插入/删除 (key, rid)。
    // key_value_of_col(col_id) 返回该行在某列上的值（double）。供执行器在写数据页后调用。
    auto MaintainIndexInsert(table_id_t table_id,
                             const std::function<IndexKeyVal(col_id_t)> &col_value, const RID &rid) -> void;
    auto MaintainIndexDelete(table_id_t table_id,
                             const std::function<IndexKeyVal(col_id_t)> &col_value, const RID &rid) -> void;

    auto LoadFromDisk() -> void;
    auto InitFreshDisk() -> void;

    std::unordered_map<std::string, table_id_t> table_name_map_;
    std::unordered_map<table_id_t, TableCatalogEntry> table_entry_map_;
    std::unordered_map<table_id_t, std::unique_ptr<SchemaPage>> schema_map_;

    // 索引注册表：index_id -> IndexInfo；以及 名字->id、表->索引id列表 的辅助映射。
    std::unordered_map<uint32_t, IndexInfo> index_map_;
    std::unordered_map<std::string, uint32_t> index_name_map_;
    std::unordered_map<table_id_t, std::vector<uint32_t>> table_index_map_;

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

    // 索引定义持久化：把 index_map_ 的全部定义写入索引 catalog 页（单页，定长记录）。
    auto PersistIndexDefs() -> void;
    // 从索引 catalog 页读回定义，重建索引实例并扫表填充。
    auto LoadIndexes() -> void;
    // 扫表填充一个（空的）索引实例。
    auto RebuildIndex(IndexInfo &info) -> void;

    std::atomic<table_id_t> next_table_id_{1};
    std::atomic<page_id_t> next_free_page_no_{1};
    std::atomic<uint32_t> next_index_id_{1};
    std::atomic<col_id_t> next_col_id_{1}; // 全局单调 col_id 分配器（跨表唯一）

    std::shared_ptr<BufferManager> buffer_manager_{nullptr};
    page_id_t current_catalog_page_no_{CATALOG_FIRST_TABLE_PAGE_NO};
    page_id_t index_catalog_page_no_{-1}; // 索引定义页（懒分配；存于 root meta）
    std::unordered_map<table_id_t, page_id_t> entry_page_map_;

    mutable std::mutex mutex_;
};

}
