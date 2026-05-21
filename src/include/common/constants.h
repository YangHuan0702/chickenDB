//
// Created by huan.yang on 2026-01-27.
//
#pragma once
#include "common/types.h"

namespace chickenDB {
#define DEFAULT_SCHEMA "main"

    constexpr int PAGE_MAGIC_NUM = 0x3f;

#define DATA_PATH "/data/chickenDB"

    // catalog 文件使用 table_id = 0，对应 /data/chickenDB/00...00.td
    constexpr table_id_t CATALOG_TABLE_ID        = 0;
    constexpr page_id_t  CATALOG_ROOT_PAGE_NO    = 0; // RootMetaPage
    constexpr page_id_t  CATALOG_FIRST_TABLE_PAGE_NO = 1; // 第一个 TableCatalogPage
    constexpr uint64_t   CATALOG_MAGIC = 0x4348494B454E4442ULL; // "CHICKNDB"

}
