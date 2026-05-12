//
// Created by 杨欢 on 2026/5/12.
//
#include "disk/posix_disk_manager.h"

using namespace chickenDB;

PosixDiskManager::PosixDiskManager(std::unique_ptr<std::fstream> fs) : fs_(std::move(fs)) {

}

PosixDiskManager::~PosixDiskManager() = default;

auto PosixDiskManager::SetPageSize(size_t page_size) -> void {
    this->page_size_ = page_size;
}

auto PosixDiskManager::WritePage(page_id_t page_id, Page *page) -> bool {
    size_t offset = page_id * page_size_;
    fs_->seekp(offset);
    fs_->write(page->data,page_size_);
    return true;
}


auto PosixDiskManager::ReadPage(page_id_t page_id, Page *page) -> bool {
    size_t offset = page_id * page_size_;
    fs_->seekg(offset);
    fs_->read(page->data,page_size_);
    return true;
}

auto PosixDiskManager::GetFileSize() -> size_t {
    return fs_->tellg();
}
