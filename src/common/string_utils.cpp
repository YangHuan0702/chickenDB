//
// Created by huan.yang on 2026-05-08.
//
#include "common/utils/string_utils.h"

#include <iomanip>
#include <sstream>

using namespace chickenDB;

auto StringUtil::FormatFileName(table_id_t table_id) -> std::string {
    std::stringstream ss;
    ss << std::setw(32) << std::setfill('0') << table_id << ".td";
    return ss.str();
}

