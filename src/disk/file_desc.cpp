//
// Created by 杨欢 on 2026/5/12.
//
#include "disk/file_desc.h"

#include <unistd.h>
#include <sys/fcntl.h>

#include "disk/posix_disk_manager.h"

using namespace chickenDB;


FileDesc::FileDesc(table_id_t table_id, const std::string& file_path) : table_id_(table_id) {
#ifdef __APPLE__
    fd_ = open(file_path.c_str(),O_RDWR|O_CREAT,0644);
    fcntl(fd_,F_NOCACHE,1);
#else
    fd_ = open(file_path.c_str(),O_RDWR|O_CREAT|O_DIRECT,0644);
#endif
    disk_manager_ = std::make_unique<PosixDiskManager>(fd_);
}

FileDesc::~FileDesc() {
    int ret = fcntl(fd_,F_GETFD);
    if (ret !=-1) {
        close(fd_);
    }
}


auto FileDesc::Close() -> void {

}
