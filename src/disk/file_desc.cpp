//
// Created by 杨欢 on 2026/5/12.
//
#include "disk/file_desc.h"

#include "disk/posix_disk_manager.h"

using namespace chickenDB;


FileDesc::FileDesc(table_id_t table_id, std::unique_ptr<std::fstream> fstream) : table_id_(table_id) {
    disk_manager_ = std::make_unique<PosixDiskManager>(std::move(fstream));
}


FileDesc::~FileDesc() = default;


auto FileDesc::Close() -> void {

}
