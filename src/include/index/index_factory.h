//
// Created by huan.yang on 2026-06-11.
//
#pragma once
#include <memory>

#include "buffer/buffer_manager.h"
#include "common/types.h"
#include "index/index.h"

namespace chickenDB {
    // 按索引类别创建具体索引实例。上层只拿 unique_ptr<Index>，不感知具体类型。
    // 新增索引类别时只需在此登记，调用方无需改动。
    class IndexFactory {
    public:
        // 纯内存索引（Hash/Bitmap，或无 buffer 时的 B+树回退）。
        static auto Create(IndexType type) -> std::unique_ptr<Index>;

        // 磁盘版 B+树索引（节点即页）。root_page_id<0 表示新建。
        // 返回的索引实例的 RootPageId() 给出实际根页，供 catalog 持久化。
        static auto CreateDiskBPlusTree(std::shared_ptr<BufferManager> buffer, table_id_t table_id,
                                        size_t key_cols, page_id_t root_page_id)
            -> std::unique_ptr<Index>;
    };
}
