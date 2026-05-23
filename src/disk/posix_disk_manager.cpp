//
// Created by 杨欢 on 2026/5/12.
//
#include "disk/posix_disk_manager.h"

#include <cstring>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/stat.h>

#include "common/chicken_execption.h"

using namespace chickenDB;

PosixDiskManager::PosixDiskManager(int fd) : fd_(fd) {
    int ret = fcntl(fd,F_GETFD);
    ChickenException::AssertCondition(ret != -1,"[PosixDiskManager] open a fail file.");
}

PosixDiskManager::~PosixDiskManager() = default;

auto PosixDiskManager::SetPageSize(size_t page_size) -> void {
    this->page_size_ = page_size;
}

auto PosixDiskManager::WritePage(page_id_t page_id, Page *page) -> bool {
    size_t offset = page_id * page_size_;
    auto r = pwrite(fd_,page->data,PAGE_SIZE,offset);
    // fs_->clear();
    // fs_->seekp(offset);
    // fs_->write(page->data,page_size_);
    // fs_->flush();
    return r;
}


auto PosixDiskManager::ReadPage(page_id_t page_id, Page *page) -> bool {
    size_t offset = page_id * page_size_;
    // fs_->clear();
    // fs_->seekg(offset);
    // fs_->read(page->data,page_size_);
    // const auto bytes_read = fs_->gcount();
    // if (bytes_read < static_cast<std::streamsize>(page_size_)) {
    //     std::memset(page->data + bytes_read, 0, page_size_ - bytes_read);
    // }
    // const bool full_page_read = bytes_read == static_cast<std::streamsize>(page_size_);
    // fs_->clear();
    auto r = pread(fd_,page->data,PAGE_SIZE,offset);
    return r;
}

auto PosixDiskManager::GetFileSize() -> size_t {
    // fs_->clear();
    // const auto current = fs_->tellg();
    // fs_->seekg(0, std::ios::end);
    // const auto size = fs_->tellg();
    // fs_->seekg(current);
    struct stat stat_buf{};
    fstat(fd_,&stat_buf);
    return stat_buf.st_size;
}
