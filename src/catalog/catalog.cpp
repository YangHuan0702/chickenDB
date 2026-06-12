#include "catalog/catalog.h"

#include <algorithm>
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
#include "index/index_factory.h"
#include "index/index_key.h"
#include "index/disk_b_plus_tree.h"
#include "buffer/table_scan_iterator.h"
#include "executor/chunk.h"

namespace chickenDB {

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

Catalog::Catalog(std::shared_ptr<BufferManager> buffer_manager) : buffer_manager_(std::move(buffer_manager)) {
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

    // 索引定义页（存在 root meta 的 free_list_page_id 槽；0 表示尚无索引）。
    index_catalog_page_no_ = root_info.free_list_page_id != 0
                                 ? static_cast<page_id_t>(root_info.free_list_page_id)
                                 : -1;

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

    // 重算全局 col_id 计数器 = 所有已加载 schema 的最大 col_id + 1。
    for (const auto &kv : schema_map_) {
        for (const auto &col : kv.second->columns_) {
            if (col.col_id >= next_col_id_) next_col_id_ = col.col_id + 1;
        }
    }

    // 还原索引：读定义 + 扫表重建内存索引。
    LoadIndexes();
}


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

auto Catalog::AddRowCount(table_id_t table_id, uint64_t delta) -> void {
    std::unique_lock<std::shared_mutex> lock(rw_mutex_);
    auto it = table_entry_map_.find(table_id);
    if (it == table_entry_map_.end()) return;
    it->second.row_count += delta;
    PersistUpdatedEntry(table_id);
}

auto Catalog::CreateIndex(const std::string &index_name, table_id_t table_id,
                          const std::vector<col_id_t> &key_cols, IndexType type,
                          bool unique) -> uint32_t {
    std::unique_lock<std::shared_mutex> lock(rw_mutex_);
    ChickenException::AssertCondition(!index_name.empty(), "index name can not be empty");
    ChickenException::AssertCondition(index_name_map_.find(index_name) == index_name_map_.end(),
                                      "index already exists: " + index_name);
    ChickenException::AssertCondition(table_entry_map_.find(table_id) != table_entry_map_.end(),
                                      "[CreateIndex] unknown table");
    ChickenException::AssertCondition(!key_cols.empty(), "index must have at least one key column");

    const uint32_t index_id = next_index_id_++;
    IndexInfo info;
    info.index_id = index_id;
    info.index_name = index_name;
    info.table_id = table_id;
    info.key_cols = key_cols;
    info.type = type;
    info.unique = unique;
    info.root_page_id = -1;

    // B+树 + 磁盘模式：用磁盘版（节点即页，重启不需扫表重建）。其余用内存版。
    if (type == IndexType::BPlusTree && buffer_manager_ != nullptr) {
        auto disk = IndexFactory::CreateDiskBPlusTree(buffer_manager_, table_id, key_cols.size(), -1);
        info.root_page_id = static_cast<DiskBPlusTreeIndex *>(disk.get())->RootPageId();
        info.index = std::move(disk);
    } else {
        info.index = IndexFactory::Create(type);
    }

    index_map_.emplace(index_id, std::move(info));
    index_name_map_.emplace(index_name, index_id);
    table_index_map_[table_id].push_back(index_id);
    PersistIndexDefs(); // 索引定义落盘（磁盘模式）
    return index_id;
}

auto Catalog::GetTableIndexes(table_id_t table_id) const -> std::vector<const IndexInfo *> {
    std::shared_lock<std::shared_mutex> lock(rw_mutex_);
    std::vector<const IndexInfo *> out;
    auto it = table_index_map_.find(table_id);
    if (it == table_index_map_.end()) return out;
    for (uint32_t id : it->second) {
        auto iit = index_map_.find(id);
        if (iit != index_map_.end()) out.push_back(&iit->second);
    }
    return out;
}

auto Catalog::GetIndex(const std::string &index_name) const -> const IndexInfo * {
    std::shared_lock<std::shared_mutex> lock(rw_mutex_);
    auto it = index_name_map_.find(index_name);
    if (it == index_name_map_.end()) return nullptr;
    auto iit = index_map_.find(it->second);
    return iit == index_map_.end() ? nullptr : &iit->second;
}

auto Catalog::MaintainIndexInsert(table_id_t table_id,
                                  const std::function<double(col_id_t)> &col_value,
                                  const RID &rid) -> void {
    std::shared_lock<std::shared_mutex> lock(rw_mutex_);
    auto tit = table_index_map_.find(table_id);
    if (tit == table_index_map_.end()) return;
    for (uint32_t id : tit->second) {
        auto iit = index_map_.find(id);
        if (iit == index_map_.end() || iit->second.index == nullptr) continue;
        std::vector<double> vals;
        for (col_id_t c : iit->second.key_cols) vals.push_back(col_value(c));
        iit->second.index->Insert(IndexKey(vals), rid);
    }
}

auto Catalog::MaintainIndexDelete(table_id_t table_id,
                                  const std::function<double(col_id_t)> &col_value,
                                  const RID &rid) -> void {
    std::shared_lock<std::shared_mutex> lock(rw_mutex_);
    auto tit = table_index_map_.find(table_id);
    if (tit == table_index_map_.end()) return;
    for (uint32_t id : tit->second) {
        auto iit = index_map_.find(id);
        if (iit == index_map_.end() || iit->second.index == nullptr) continue;
        std::vector<double> vals;
        for (col_id_t c : iit->second.key_cols) vals.push_back(col_value(c));
        iit->second.index->Erase(IndexKey(vals), rid);
    }
}

auto Catalog::PersistRootMeta() -> void {
    if (buffer_manager_ == nullptr) return;

    RootMetaPageStruct info{};
    info.magic           = CATALOG_MAGIC;
    info.version_major   = 1;
    info.version_minor   = 0;
    info.catalog_page_id = static_cast<uint32_t>(CATALOG_FIRST_TABLE_PAGE_NO);
    info.total_pages     = static_cast<uint64_t>(next_free_page_no_);
    // 索引定义页号存在 free_list_page_id 槽（未做空闲页链表，复用之）。
    info.free_list_page_id = index_catalog_page_no_ >= 0
                                 ? static_cast<uint32_t>(index_catalog_page_no_) : 0;

    Page *page = buffer_manager_->FetchPage(CATALOG_TABLE_ID, CATALOG_ROOT_PAGE_NO);
    SerializeRootMeta(page, info);
    buffer_manager_->UnpinPage(CATALOG_TABLE_ID, CATALOG_ROOT_PAGE_NO, true);
}


auto Catalog::CreateTable(const std::string& table_name,const std::vector<ColumnDefine>& colums_, uint64_t create_ts) -> TableCatalogEntry {
    std::unique_lock<std::shared_mutex> lock(rw_mutex_);

    ChickenException::AssertCondition(!table_name.empty(), "table name can not be empty");
    ChickenException::AssertCondition(GetTableLocked(table_name) == nullptr,
                                      "table already exists: " + table_name);
    ChickenException::AssertCondition(!colums_.empty(),
                                      "table must contain at least one column");

    const auto table_id = AllocateTableId();
    auto schema = BuildInitialSchema(table_name,colums_, create_ts);

    TableCatalogEntry entry;
    entry.table_id   = table_id;
    entry.SetTableName(table_name);
    entry.create_ts  = create_ts;

    if (buffer_manager_ != nullptr) {
        // 磁盘模式：先写 schema 页拿到真实页号
        entry.schema_page_id = static_cast<uint32_t>(PersistSchema(*schema));
    } else {
        // 纯内存模式：顺序分配 ID（从 1 开始，与旧行为一致）
        entry.schema_page_id = static_cast<uint32_t>(AllocateSchemaPageNo());
    }

    table_name_map_.emplace(table_name, table_id);
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


auto Catalog::AllocateTableId() -> table_id_t {
    return next_table_id_++;
}

auto Catalog::AllocateSchemaPageNo() -> page_id_t {
    return next_free_page_no_++;
}

auto Catalog::BuildInitialSchema(const std::string& table_name,const std::vector<ColumnDefine>& colums_, uint64_t create_ts)
    -> std::unique_ptr<SchemaPage> {
    auto schema = std::make_unique<SchemaPage>();
    schema->version_.version      = 1;
    schema->version_.effective_ts = create_ts;

    // col_id 全局唯一（跨表单调递增），使多表 JOIN 后左右列 col_id 不冲突，
    // 表达式求值/列映射（按 col_id）无需感知列属于哪张表。
    for (const auto &column : colums_) {
        ColDef col_def;
        col_def.col_id      = next_col_id_++;
        col_def.SetColumnName(column.name_);
        col_def.data_type   = column.type_;
        col_def.type_param  = static_cast<uint16_t>(column.size_);
        schema->AddColumn(col_def);
    }
    return schema;
}

// 索引定义页布局：[uint32 count][IndexDefRecord ...]。单页，足够数十个索引。
auto Catalog::PersistIndexDefs() -> void {
    if (buffer_manager_ == nullptr) return;

    if (index_catalog_page_no_ < 0) {
        Page *p = AllocateDiskPage();
        index_catalog_page_no_ = p->page_id_.page_no;
        buffer_manager_->UnpinPage(CATALOG_TABLE_ID, index_catalog_page_no_, true);
        PersistRootMeta(); // 记录索引页号
    }

    Page *page = buffer_manager_->FetchPage(CATALOG_TABLE_ID, index_catalog_page_no_);
    size_t offset = 0;
    uint32_t count = static_cast<uint32_t>(index_map_.size());
    std::memcpy(page->data + offset, &count, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    for (const auto &kv : index_map_) {
        const IndexInfo &info = kv.second;
        IndexDefRecord rec{};
        rec.index_id = info.index_id;
        std::memset(rec.index_name, 0, INDEX_NAME_MAX_LEN);
        std::memcpy(rec.index_name, info.index_name.data(),
                    std::min(info.index_name.size(), INDEX_NAME_MAX_LEN - 1));
        rec.table_id = info.table_id;
        rec.type = static_cast<uint8_t>(info.type);
        rec.unique = info.unique ? 1 : 0;
        rec.key_count = static_cast<uint8_t>(std::min(info.key_cols.size(), INDEX_MAX_KEY_COLS));
        for (uint8_t i = 0; i < rec.key_count; i++) rec.key_cols[i] = info.key_cols[i];
        rec.root_page_id = info.root_page_id;
        std::memcpy(page->data + offset, &rec, sizeof(IndexDefRecord));
        offset += sizeof(IndexDefRecord);
    }
    buffer_manager_->UnpinPage(CATALOG_TABLE_ID, index_catalog_page_no_, true);
}

auto Catalog::LoadIndexes() -> void {
    if (buffer_manager_ == nullptr || index_catalog_page_no_ < 0) return;

    Page *page = buffer_manager_->FetchPage(CATALOG_TABLE_ID, index_catalog_page_no_);
    size_t offset = 0;
    uint32_t count = 0;
    std::memcpy(&count, page->data + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    for (uint32_t i = 0; i < count; i++) {
        IndexDefRecord rec{};
        std::memcpy(&rec, page->data + offset, sizeof(IndexDefRecord));
        offset += sizeof(IndexDefRecord);

        IndexInfo info;
        info.index_id = rec.index_id;
        info.index_name = std::string(rec.index_name, strnlen(rec.index_name, INDEX_NAME_MAX_LEN));
        info.table_id = rec.table_id;
        info.type = static_cast<IndexType>(rec.type);
        info.unique = rec.unique != 0;
        for (uint8_t k = 0; k < rec.key_count; k++) info.key_cols.push_back(rec.key_cols[k]);
        info.root_page_id = rec.root_page_id;

        if (rec.index_id >= next_index_id_) next_index_id_ = rec.index_id + 1;

        if (info.type == IndexType::BPlusTree && info.root_page_id >= 0) {
            // 磁盘版 B+树：直接从根页加载，无需扫表重建。
            info.index = IndexFactory::CreateDiskBPlusTree(buffer_manager_, info.table_id,
                                                           info.key_cols.size(), info.root_page_id);
        } else {
            // 内存版（Hash/Bitmap）：新建实例并扫表重填。
            info.index = IndexFactory::Create(info.type);
            RebuildIndex(info);
        }

        const uint32_t id = info.index_id;
        const std::string name = info.index_name;
        const table_id_t tid = info.table_id;
        index_map_.emplace(id, std::move(info));
        index_name_map_.emplace(name, id);
        table_index_map_[tid].push_back(id);
    }
    buffer_manager_->UnpinPage(CATALOG_TABLE_ID, index_catalog_page_no_, false);
}

// 扫表，按索引键列构造键 + RID 填充索引实例。
auto Catalog::RebuildIndex(IndexInfo &info) -> void {
    const auto sit = schema_map_.find(info.table_id);
    if (sit == schema_map_.end()) return;
    SchemaPage *schema = sit->second.get();

    // 索引列 col_id -> schema 列下标。
    std::vector<size_t> key_idx;
    for (col_id_t cid : info.key_cols) {
        for (size_t i = 0; i < schema->columns_.size(); i++) {
            if (schema->columns_[i].col_id == cid) { key_idx.push_back(i); break; }
        }
    }

    const page_id_t page_count = buffer_manager_->GetPageCount(info.table_id);
    const page_id_t last = page_count > 0 ? page_count - 1 : 0;
    TableScanIterator it(info.table_id, 0, last, buffer_manager_, schema);

    Chunk chunk;
    while (it.Next(chunk)) {
        const page_id_t page_no = it.CurrentPageNo();
        const size_t n = chunk.Count();
        for (size_t r = 0; r < n; r++) {
            std::vector<double> vals;
            vals.reserve(key_idx.size());
            for (size_t ci : key_idx) {
                const Vector &v = chunk.GetColumn(ci);
                vals.push_back(v.GetType() == ColumnType::NUMBER
                                   ? static_cast<double>(v.GetValue<int32_t>(r))
                                   : v.GetValue<double>(r));
            }
            info.index->Insert(IndexKey(vals), RID(page_no, static_cast<uint32_t>(r)));
        }
    }
}

} // namespace chickenDB
