#include "catalog/catalog.h"

#include <cstring>
#include <utility>

#include "buffer/buffer_manager.h"
#include "catalog/col_def.h"
#include "catalog/root_meta_page.h"
#include "catalog/schema_version.h"
#include "catalog/table_catalog.h"
#include "common/chicken_execption.h"
#include "common/constants.h"
#include "common/macro.h"

namespace chickenDB {

// ─── 序列化辅助 ────────────────────────────────────────────────────────────
// 所有结构体均为 POD，直接 memcpy 到 Page::data。
//
// RootMetaPage 布局：data[0 .. sizeof(RootMetaPageStruct)]
// TableCatalogPage 布局：[TableCatalogPageHeader][TableCatalogEntry × N]
// SchemaPage 布局：[SchemaVersion][ColDef × N]
// ──────────────────────────────────────────────────────────────────────────

static void SerializeRootMeta(Page *page, const RootMetaPageStruct &info) {
    std::memcpy(page->data, &info, sizeof(RootMetaPageStruct));
}

static void DeserializeRootMeta(const Page *page, RootMetaPageStruct &info) {
    std::memcpy(&info, page->data, sizeof(RootMetaPageStruct));
}

struct CatalogPageData {
    TableCatalogPageHeader header{};
    std::vector<TableCatalogEntry> entries{};
};

static void SerializeCatalogPage(Page *page, const CatalogPageData &cpd) {
    std::memcpy(page->data, &cpd.header, sizeof(TableCatalogPageHeader));
    for (size_t i = 0; i < cpd.entries.size(); ++i) {
        std::memcpy(page->data + sizeof(TableCatalogPageHeader) + i * sizeof(TableCatalogEntry),
                    &cpd.entries[i], sizeof(TableCatalogEntry));
    }
}

static void DeserializeCatalogPage(const Page *page, CatalogPageData &cpd) {
    std::memcpy(&cpd.header, page->data, sizeof(TableCatalogPageHeader));
    cpd.entries.resize(cpd.header.entry_count);
    for (uint16_t i = 0; i < cpd.header.entry_count; ++i) {
        std::memcpy(&cpd.entries[i],
                    page->data + sizeof(TableCatalogPageHeader) + i * sizeof(TableCatalogEntry),
                    sizeof(TableCatalogEntry));
    }
}

static void SerializeSchema(Page *page, const SchemaPage &schema) {
    std::memcpy(page->data, &schema.version_, sizeof(SchemaVersion));
    for (size_t i = 0; i < schema.columns_.size(); ++i) {
        std::memcpy(page->data + sizeof(SchemaVersion) + i * sizeof(ColDef),
                    &schema.columns_[i], sizeof(ColDef));
    }
}

static void DeserializeSchema(const Page *page, SchemaPage &schema) {
    std::memcpy(&schema.version_, page->data, sizeof(SchemaVersion));
    schema.columns_.resize(schema.version_.col_count);
    for (uint16_t i = 0; i < schema.version_.col_count; ++i) {
        std::memcpy(&schema.columns_[i],
                    page->data + sizeof(SchemaVersion) + i * sizeof(ColDef),
                    sizeof(ColDef));
    }
}

// 一个 256KB 页最多能放多少条 TableCatalogEntry
static constexpr size_t kMaxEntriesPerCatalogPage =
    (PAGE_SIZE - sizeof(TableCatalogPageHeader)) / sizeof(TableCatalogEntry);

// ─── 构造函数 ──────────────────────────────────────────────────────────────

Catalog::Catalog(BufferManager *buffer_manager) : buffer_manager_(buffer_manager) {
    // 磁盘模式：schema 页从 page 2 开始（0=root, 1=catalog 链表头）
    next_free_page_no_ = 2;
    LoadFromDisk();
}

// ─── 磁盘生命周期 ──────────────────────────────────────────────────────────

auto Catalog::InitFreshDisk() -> void {
    // Page 0：根元数据页
    RootMetaPageStruct root_info{};
    root_info.magic             = CATALOG_MAGIC;
    root_info.version_major     = 1;
    root_info.version_minor     = 0;
    root_info.catalog_page_id   = static_cast<uint32_t>(CATALOG_FIRST_TABLE_PAGE_NO);
    root_info.total_pages       = 2; // root + 第一个 catalog 页

    Page *root_page = AllocateDiskPage(); // page_no = 0
    SerializeRootMeta(root_page, root_info);
    buffer_manager_->UnpinPage(CATALOG_TABLE_ID, root_page->page_id_.page_no, true);

    // Page 1：空的 TableCatalogPage
    Page *cat_page = AllocateDiskPage(); // page_no = 1
    CatalogPageData empty_cpd{};
    SerializeCatalogPage(cat_page, empty_cpd);
    buffer_manager_->UnpinPage(CATALOG_TABLE_ID, cat_page->page_id_.page_no, true);

    current_catalog_page_no_ = CATALOG_FIRST_TABLE_PAGE_NO;
    // next_free_page_no_ 已在 AllocateDiskPage 中同步到 2
}

auto Catalog::LoadFromDisk() -> void {
    // 读根元数据页，判断是新文件还是已有数据
    Page *root_page = buffer_manager_->FetchPage(CATALOG_TABLE_ID, CATALOG_ROOT_PAGE_NO);
    RootMetaPageStruct root_info{};
    DeserializeRootMeta(root_page, root_info);
    buffer_manager_->UnpinPage(CATALOG_TABLE_ID, CATALOG_ROOT_PAGE_NO, false);

    if (root_info.magic != CATALOG_MAGIC) {
        // 全新文件，初始化磁盘布局
        InitFreshDisk();
        return;
    }

    // 恢复页号计数器，让 BufferManager::NewPage 从正确位置分配
    next_free_page_no_ = static_cast<page_id_t>(root_info.total_pages);
    buffer_manager_->InitNextPageNo(CATALOG_TABLE_ID, next_free_page_no_);

    // 遍历 TableCatalogPage 链表，还原内存 map
    page_id_t cat_page_no = static_cast<page_id_t>(root_info.catalog_page_id);
    while (cat_page_no > 0) {
        Page *cat_page = buffer_manager_->FetchPage(CATALOG_TABLE_ID, cat_page_no);
        CatalogPageData cpd{};
        DeserializeCatalogPage(cat_page, cpd);
        buffer_manager_->UnpinPage(CATALOG_TABLE_ID, cat_page_no, false);

        for (const auto &entry : cpd.entries) {
            entry_page_map_[entry.table_id] = cat_page_no;
            table_entry_map_[entry.table_id] = entry;

            if (entry.IsActive()) {
                table_name_map_[entry.GetTableName()] = entry.table_id;
                if (entry.table_id >= next_table_id_) {
                    next_table_id_ = entry.table_id + 1;
                }

                // 读取对应的 SchemaPage
                auto schema = std::make_unique<SchemaPage>();
                Page *schema_page = buffer_manager_->FetchPage(
                    CATALOG_TABLE_ID, static_cast<page_id_t>(entry.schema_page_id));
                DeserializeSchema(schema_page, *schema);
                buffer_manager_->UnpinPage(
                    CATALOG_TABLE_ID, static_cast<page_id_t>(entry.schema_page_id), false);
                schema_map_[entry.table_id] = std::move(schema);
            }
        }

        current_catalog_page_no_ = cat_page_no;
        // next_page_id == 0 表示链表末尾
        cat_page_no = (cpd.header.next_page_id != 0)
                          ? static_cast<page_id_t>(cpd.header.next_page_id)
                          : 0;
    }
}

// ─── 磁盘辅助函数 ──────────────────────────────────────────────────────────

auto Catalog::AllocateDiskPage() -> Page * {
    Page *page = buffer_manager_->NewPage(CATALOG_TABLE_ID);
    // 保持本地计数与 BufferManager 同步
    next_free_page_no_ = page->page_id_.page_no + 1;
    return page;
}

auto Catalog::PersistSchema(const SchemaPage &schema) -> page_id_t {
    Page *page = AllocateDiskPage();
    page_id_t schema_page_no = page->page_id_.page_no;
    SerializeSchema(page, schema);
    buffer_manager_->UnpinPage(CATALOG_TABLE_ID, schema_page_no, true);
    return schema_page_no;
}

auto Catalog::PersistNewEntry(const TableCatalogEntry &entry) -> void {
    if (buffer_manager_ == nullptr) return;

    Page *cat_page = buffer_manager_->FetchPage(CATALOG_TABLE_ID, current_catalog_page_no_);
    CatalogPageData cpd{};
    DeserializeCatalogPage(cat_page, cpd);

    if (cpd.entries.size() >= kMaxEntriesPerCatalogPage) {
        // 当前页已满：分配新页，链入链表
        page_id_t new_page_no = next_free_page_no_; // AllocateDiskPage 会分配此值
        cpd.header.next_page_id = static_cast<uint32_t>(new_page_no);
        SerializeCatalogPage(cat_page, cpd);
        buffer_manager_->UnpinPage(CATALOG_TABLE_ID, current_catalog_page_no_, true);

        Page *new_page = AllocateDiskPage(); // page_no == new_page_no
        current_catalog_page_no_ = new_page->page_id_.page_no;
        cpd = {};
        cat_page = new_page;
    }

    entry_page_map_[entry.table_id] = current_catalog_page_no_;
    cpd.entries.push_back(entry);
    cpd.header.entry_count = static_cast<uint16_t>(cpd.entries.size());
    SerializeCatalogPage(cat_page, cpd);
    buffer_manager_->UnpinPage(CATALOG_TABLE_ID, current_catalog_page_no_, true);

    PersistRootMeta();
}

auto Catalog::PersistUpdatedEntry(table_id_t table_id) -> void {
    if (buffer_manager_ == nullptr) return;

    auto page_it = entry_page_map_.find(table_id);
    if (page_it == entry_page_map_.end()) return;

    page_id_t page_no = page_it->second;
    Page *cat_page = buffer_manager_->FetchPage(CATALOG_TABLE_ID, page_no);
    CatalogPageData cpd{};
    DeserializeCatalogPage(cat_page, cpd);

    for (auto &e : cpd.entries) {
        if (e.table_id == table_id) {
            e = table_entry_map_.at(table_id);
            break;
        }
    }

    SerializeCatalogPage(cat_page, cpd);
    buffer_manager_->UnpinPage(CATALOG_TABLE_ID, page_no, true);
}

auto Catalog::PersistRootMeta() -> void {
    if (buffer_manager_ == nullptr) return;

    RootMetaPageStruct info{};
    info.magic           = CATALOG_MAGIC;
    info.version_major   = 1;
    info.version_minor   = 0;
    info.catalog_page_id = static_cast<uint32_t>(CATALOG_FIRST_TABLE_PAGE_NO);
    info.total_pages     = static_cast<uint64_t>(next_free_page_no_);

    Page *page = buffer_manager_->FetchPage(CATALOG_TABLE_ID, CATALOG_ROOT_PAGE_NO);
    SerializeRootMeta(page, info);
    buffer_manager_->UnpinPage(CATALOG_TABLE_ID, CATALOG_ROOT_PAGE_NO, true);
}

// ─── 表操作 ────────────────────────────────────────────────────────────────

auto Catalog::CreateTable(const CreateTableStatement &statement, uint64_t create_ts) -> TableCatalogEntry {
    std::unique_lock<std::shared_mutex> lock(rw_mutex_);

    ChickenException::AssertCondition(!statement.table_name_.empty(), "table name can not be empty");
    ChickenException::AssertCondition(GetTableLocked(statement.table_name_) == nullptr,
                                      "table already exists: " + statement.table_name_);
    ChickenException::AssertCondition(!statement.columns_.empty(),
                                      "table must contain at least one column");

    const auto table_id = AllocateTableId();
    auto schema = BuildInitialSchema(statement, create_ts);

    TableCatalogEntry entry;
    entry.table_id   = table_id;
    entry.SetTableName(statement.table_name_);
    entry.create_ts  = create_ts;

    if (buffer_manager_ != nullptr) {
        // 磁盘模式：先写 schema 页拿到真实页号
        entry.schema_page_id = static_cast<uint32_t>(PersistSchema(*schema));
    } else {
        // 纯内存模式：顺序分配 ID（从 1 开始，与旧行为一致）
        entry.schema_page_id = static_cast<uint32_t>(AllocateSchemaPageNo());
    }

    table_name_map_.emplace(statement.table_name_, table_id);
    table_entry_map_.emplace(table_id, entry);
    schema_map_.emplace(table_id, std::move(schema));

    PersistNewEntry(entry); // 磁盘模式写 catalog 页；纯内存为 no-op

    return entry;
}

auto Catalog::DropTable(const std::string &table_name, uint64_t drop_ts) -> bool {
    std::unique_lock<std::shared_mutex> lock(rw_mutex_);

    const auto table_iter = table_name_map_.find(table_name); // 直接查 map，无需再调 GetTableLocked
    if (table_iter == table_name_map_.end()) return false;

    const auto entry_iter = table_entry_map_.find(table_iter->second);
    if (entry_iter == table_entry_map_.end()) {
        table_name_map_.erase(table_iter);
        return false;
    }

    const table_id_t tid = entry_iter->first;
    entry_iter->second.status  = TableCatalogEntryStatus::DROPPED;
    entry_iter->second.drop_ts = drop_ts;
    table_name_map_.erase(table_iter);

    PersistUpdatedEntry(tid); // 把改过的 entry 刷回磁盘

    return true;
}

// ─── 无锁内部查询（调用方须已持锁）────────────────────────────────────────

auto Catalog::GetTableLocked(const std::string &table_name) const -> const TableCatalogEntry * {
    const auto it = table_name_map_.find(table_name);
    if (it == table_name_map_.end()) return nullptr;
    return GetTableLocked(it->second);
}

auto Catalog::GetTableLocked(table_id_t table_id) const -> const TableCatalogEntry * {
    const auto it = table_entry_map_.find(table_id);
    if (it == table_entry_map_.end() || !it->second.IsActive()) return nullptr;
    return &it->second;
}

// ─── 公共查询接口（加锁后委托内部方法）────────────────────────────────────

auto Catalog::GetTable(const std::string &table_name) const -> const TableCatalogEntry * {
    std::shared_lock<std::shared_mutex> lock(rw_mutex_);
    return GetTableLocked(table_name);
}

auto Catalog::GetTable(table_id_t table_id) const -> const TableCatalogEntry * {
    std::shared_lock<std::shared_mutex> lock(rw_mutex_);
    return GetTableLocked(table_id);
}

auto Catalog::GetSchema(table_id_t table_id) const -> const SchemaPage * {
    std::shared_lock<std::shared_mutex> lock(rw_mutex_);

    if (GetTableLocked(table_id) == nullptr) return nullptr;
    const auto it = schema_map_.find(table_id);
    if (it == schema_map_.end()) return nullptr;
    return it->second.get();
}

auto Catalog::TableExists(const std::string &table_name) const -> bool {
    std::shared_lock<std::shared_mutex> lock(rw_mutex_);
    return GetTableLocked(table_name) != nullptr;
}

// ─── 内部分配 ──────────────────────────────────────────────────────────────

auto Catalog::AllocateTableId() -> table_id_t {
    return next_table_id_++;
}

auto Catalog::AllocateSchemaPageNo() -> page_id_t {
    return next_free_page_no_++;
}

auto Catalog::BuildInitialSchema(const CreateTableStatement &statement, uint64_t create_ts)
    -> std::unique_ptr<SchemaPage> {
    auto schema = std::make_unique<SchemaPage>();
    schema->version_.version      = 1;
    schema->version_.effective_ts = create_ts;

    col_id_t next_col_id = 1;
    for (const auto &column : statement.columns_) {
        ColDef col_def;
        col_def.col_id      = next_col_id++;
        col_def.SetColumnName(column.name_);
        col_def.data_type   = column.type_;
        col_def.type_param  = static_cast<uint16_t>(column.size_);
        schema->AddColumn(col_def);
    }
    return schema;
}

} // namespace chickenDB
