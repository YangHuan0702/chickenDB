//
// Created by 杨欢 on 2026/5/12.
//

#pragma once
#include <iosfwd>
#include <memory>

#include "disk_manager.h"
#include "common/types.h"

namespace chickenDB {

    class FileDesc {
    public:
        explicit FileDesc(table_id_t table_id,const std::string& file_path) ;
        ~FileDesc();

        auto Close() -> void;

        size_t page_size_{PAGE_SIZE};
        table_id_t table_id_;
        std::unique_ptr<DiskManager> disk_manager_;
        int fd_;
    };

}
