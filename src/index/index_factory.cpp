//
// Created by huan.yang on 2026-06-11.
//
#include "index/index_factory.h"

#include "index/b_plus_tree.h"
#include "index/hash_index.h"
#include "index/bitmap_index.h"
#include "index/disk_b_plus_tree.h"
#include "common/chicken_execption.h"

using namespace chickenDB;

auto IndexFactory::Create(IndexType type) -> std::unique_ptr<Index> {
    switch (type) {
        case IndexType::BPlusTree: return std::make_unique<BPlusTreeIndex>();
        case IndexType::Hash:      return std::make_unique<HashIndex>();
        case IndexType::Bitmap:    return std::make_unique<BitmapIndex>();
    }
    throw ChickenException("[IndexFactory] unknown index type");
}

auto IndexFactory::CreateDiskBPlusTree(std::shared_ptr<BufferManager> buffer, table_id_t table_id,
                                       size_t key_cols, page_id_t root_page_id)
    -> std::unique_ptr<Index> {
    return std::make_unique<DiskBPlusTreeIndex>(std::move(buffer), table_id, key_cols, root_page_id);
}
