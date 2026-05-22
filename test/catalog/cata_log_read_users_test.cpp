//
// Created by huan.yang on 2026-05-22.
//
#include "buffer/buffer_manager.h"
#include "catalog/catalog.h"
#include "disk/table_manager.h"
#include "gtest/gtest.h"

using namespace chickenDB;

TEST(Catalog,CatalogReadUserTest) {
    auto lru_table_manager = std::make_shared<LRUTableManager>();
    auto buffer_manager = std::make_shared<BufferManager>(lru_table_manager);

    auto catalog = std::make_shared<Catalog>(buffer_manager);

    std::string table_name = "users";
    auto table_catalog_entry = catalog->GetTable(table_name);
    table_id_t id = table_catalog_entry->table_id;
    std::cout << "table: "<< table_name <<", target id: " << id << std::endl;
}