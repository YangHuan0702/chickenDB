//
// Created by huan.yang on 2026-05-25.
//
#include "buffer/table_data_page.h"

#include "common/chicken_execption.h"

using namespace chickenDB;

auto TableDataPage::Init() -> void {
    InitHeader();
    InitCols();
    inited = true;
}


auto TableDataPage::RowCount() const -> size_t {
    ChickenException::AssertCondition(inited, "TableDataPage is not inited");
    return header_.num_rows;
}

auto TableDataPage::InitCols() -> void {
    size_t read_offset = header_.col_dir_offset;
    for (size_t i = 0; i < header_.num_columns; i++) {
        col_id_t col_id = 0;
        memcpy(&col_id,page_->data + read_offset,sizeof(uint32_t));
        read_offset += sizeof(uint32_t);

        uint32_t data_offset;
        memcpy(&data_offset,page_->data + read_offset,sizeof(uint32_t));
        read_offset += sizeof(uint32_t);

        uint32_t compressed_size;
        memcpy(&compressed_size,page_->data + read_offset,sizeof(uint32_t));
        read_offset += sizeof(uint32_t);

        uint32_t raw_size;
        memcpy(&raw_size,page_->data + read_offset,sizeof(uint32_t));
        read_offset += sizeof(uint32_t);

        ColDirEntry col_dir_entry{col_id,data_offset,compressed_size,raw_size};
        cols_.push_back(col_dir_entry);
    }
}


auto TableDataPage::InitHeader() -> void {
    size_t offset = 0;
    memcpy(&header_.magic,page_->data + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(&header_.version,page_->data + offset, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    memcpy(&header_.page_type,page_->data + offset, sizeof(uint8_t));
    offset += sizeof(uint8_t);

    memcpy(&header_.compression,page_->data + offset, sizeof(uint8_t));
    offset += sizeof(uint8_t);

    memcpy(&header_.page_id,page_->data + offset, sizeof(page_id_t));
    offset += sizeof(page_id_t);

    memcpy(&header_.num_rows,page_->data + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(&header_.num_columns,page_->data + offset, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    memcpy(&header_.col_dir_offset,page_->data + offset, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    memcpy(&header_.data_offset,page_->data + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(&header_.null_bitmap_offset,page_->data + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(&header_.min_row_id,page_->data + offset, sizeof(uint64_t));
    offset += sizeof(uint64_t);

    memcpy(&header_.max_row_id,page_->data + offset, sizeof(uint64_t));
    offset += sizeof(uint64_t);

    memcpy(&header_.create_ts,page_->data + offset, sizeof(uint64_t));
    // offset += sizeof(uint64_t);
}


auto TableDataPage::FreeSpace() const -> size_t {
    return 0;
}

