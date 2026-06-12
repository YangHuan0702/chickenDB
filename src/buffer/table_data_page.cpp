//
// Created by huan.yang on 2026-05-25.
//
#include "buffer/table_data_page.h"

#include <cstring>

#include "common/chicken_execption.h"

using namespace chickenDB;

auto TableDataPage::Init() -> void {
    InitHeader();
    // 只有合法数据页才解析列目录；空页/非数据页 num_columns 可能是垃圾值。
    if (IsDataPage()) {
        InitCols();
    }
    inited = true;
}


auto TableDataPage::RowCount() const -> size_t {
    ChickenException::AssertCondition(inited, "TableDataPage is not inited");
    return header_.num_rows;
}

auto TableDataPage::InitCols() -> void {
    cols_.clear();
    size_t read_offset = header_.col_dir_offset;
    for (size_t i = 0; i < header_.num_columns; i++) {
        col_id_t col_id = 0;
        memcpy(&col_id, page_->data + read_offset, sizeof(uint32_t));
        read_offset += sizeof(uint32_t);

        uint32_t data_offset;
        memcpy(&data_offset, page_->data + read_offset, sizeof(uint32_t));
        read_offset += sizeof(uint32_t);

        uint32_t compressed_size;
        memcpy(&compressed_size, page_->data + read_offset, sizeof(uint32_t));
        read_offset += sizeof(uint32_t);

        uint32_t raw_size;
        memcpy(&raw_size, page_->data + read_offset, sizeof(uint32_t));
        read_offset += sizeof(uint32_t);

        ColDirEntry col_dir_entry{col_id, data_offset, compressed_size, raw_size};
        cols_.push_back(col_dir_entry);
    }
}


auto TableDataPage::InitHeader() -> void {
    size_t offset = 0;
    memcpy(&header_.magic, page_->data + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(&header_.version, page_->data + offset, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    memcpy(&header_.page_type, page_->data + offset, sizeof(uint8_t));
    offset += sizeof(uint8_t);

    memcpy(&header_.compression, page_->data + offset, sizeof(uint8_t));
    offset += sizeof(uint8_t);

    memcpy(&header_.page_id, page_->data + offset, sizeof(page_id_t));
    offset += sizeof(page_id_t);

    memcpy(&header_.num_rows, page_->data + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(&header_.num_columns, page_->data + offset, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    memcpy(&header_.col_dir_offset, page_->data + offset, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    memcpy(&header_.data_offset, page_->data + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(&header_.null_bitmap_offset, page_->data + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(&header_.min_row_id, page_->data + offset, sizeof(uint64_t));
    offset += sizeof(uint64_t);

    memcpy(&header_.max_row_id, page_->data + offset, sizeof(uint64_t));
    offset += sizeof(uint64_t);

    memcpy(&header_.create_ts, page_->data + offset, sizeof(uint64_t));
    offset += sizeof(uint64_t);

    memcpy(&header_.checksum, page_->data + offset, sizeof(uint32_t));
}


auto TableDataPage::WriteHeader() -> void {
    size_t offset = 0;
    memcpy(page_->data + offset, &header_.magic, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(page_->data + offset, &header_.version, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    memcpy(page_->data + offset, &header_.page_type, sizeof(uint8_t));
    offset += sizeof(uint8_t);

    memcpy(page_->data + offset, &header_.compression, sizeof(uint8_t));
    offset += sizeof(uint8_t);

    memcpy(page_->data + offset, &header_.page_id, sizeof(page_id_t));
    offset += sizeof(page_id_t);

    memcpy(page_->data + offset, &header_.num_rows, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(page_->data + offset, &header_.num_columns, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    memcpy(page_->data + offset, &header_.col_dir_offset, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    memcpy(page_->data + offset, &header_.data_offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(page_->data + offset, &header_.null_bitmap_offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(page_->data + offset, &header_.min_row_id, sizeof(uint64_t));
    offset += sizeof(uint64_t);

    memcpy(page_->data + offset, &header_.max_row_id, sizeof(uint64_t));
    offset += sizeof(uint64_t);

    memcpy(page_->data + offset, &header_.create_ts, sizeof(uint64_t));
    offset += sizeof(uint64_t);

    memcpy(page_->data + offset, &header_.checksum, sizeof(uint32_t));
}

// FNV-1a 32 位校验，覆盖 [from, to) 区间字节。
static auto Fnv1a(const char *data, size_t from, size_t to) -> uint32_t {
    uint32_t h = 2166136261u;
    for (size_t i = from; i < to; i++) {
        h ^= static_cast<uint8_t>(data[i]);
        h *= 16777619u;
    }
    return h;
}
                                     CompressionType compression, uint64_t base_row_id,
                                     uint64_t create_ts) -> bool {
    const auto num_columns = static_cast<uint16_t>(cols.size());
    const uint16_t col_dir_offset = K_HEADER_SIZE;
    const uint32_t data_offset = col_dir_offset + num_columns * K_COL_DIR_ENTRY_SIZE;
    const size_t bitmap_bytes = (num_rows + 7) / 8;

    // 先压缩每列数据，确定各列在数据区的偏移与大小。
    std::vector<std::vector<char>> compressed(num_columns);
    std::vector<ColDirEntry> dir(num_columns);
    uint32_t cursor = data_offset;
    for (uint16_t i = 0; i < num_columns; i++) {
        const ColumnInput &in = cols[i];
        size_t csize = PageCodec::Compress(compression, in.data, in.raw_size, compressed[i]);
        dir[i] = ColDirEntry{in.col_id, cursor, static_cast<uint32_t>(csize), in.raw_size};
        cursor += static_cast<uint32_t>(csize);
    }

    // 数据区之后是 null bitmap 区：每列一段，列序排列。
    const uint32_t null_bitmap_offset = cursor;
    const uint32_t total_size = null_bitmap_offset + static_cast<uint32_t>(bitmap_bytes * num_columns);
    if (total_size > PAGE_SIZE) {
        return false; // 单页放不下：返回 false，调用方按行数切分到多页。
    }

    // 填充 header。
    header_.magic = static_cast<uint32_t>(PAGE_MAGIC_NUM);
    header_.version = 1;
    header_.page_type = static_cast<uint8_t>(PageType::DATA);
    header_.compression = static_cast<uint8_t>(compression);
    header_.page_id = page_->page_id_.page_no;
    header_.num_rows = num_rows;
    header_.num_columns = num_columns;
    header_.col_dir_offset = col_dir_offset;
    header_.data_offset = data_offset;
    header_.null_bitmap_offset = null_bitmap_offset;
    header_.min_row_id = base_row_id;
    header_.max_row_id = num_rows == 0 ? base_row_id : base_row_id + num_rows - 1;
    header_.create_ts = create_ts;
    header_.version = 2; // v2：页尾带 checksum
    header_.checksum = 0;
    WriteHeader();

    // 写列目录。
    size_t write_offset = col_dir_offset;
    for (uint16_t i = 0; i < num_columns; i++) {
        memcpy(page_->data + write_offset, &dir[i].col_id, sizeof(uint32_t));
        write_offset += sizeof(uint32_t);
        memcpy(page_->data + write_offset, &dir[i].data_offset, sizeof(uint32_t));
        write_offset += sizeof(uint32_t);
        memcpy(page_->data + write_offset, &dir[i].compressed_size, sizeof(uint32_t));
        write_offset += sizeof(uint32_t);
        memcpy(page_->data + write_offset, &dir[i].raw_size, sizeof(uint32_t));
        write_offset += sizeof(uint32_t);
    }

    // 写压缩列数据。
    for (uint16_t i = 0; i < num_columns; i++) {
        if (!compressed[i].empty()) {
            memcpy(page_->data + dir[i].data_offset, compressed[i].data(), compressed[i].size());
        }
    }

    // 写 null bitmap 区，列序排列；validity 为 nullptr 视为全有效（0xFF）。
    for (uint16_t i = 0; i < num_columns; i++) {
        char *dst = page_->data + null_bitmap_offset + i * bitmap_bytes;
        if (cols[i].validity != nullptr) {
            memcpy(dst, cols[i].validity, bitmap_bytes);
        } else {
            memset(dst, 0xFF, bitmap_bytes);
        }
    }

    // 同步内存态，使后续读 accessor 立即可用。
    cols_ = std::move(dir);
    // 计算并写入校验和（覆盖 header 之后到 total_size 的全部内容）。
    header_.checksum = Fnv1a(page_->data, K_HEADER_SIZE, total_size);
    WriteHeader();
    inited = true;
    return true;
}

// 校验页内容是否与存储的 checksum 一致（checksum==0 视为未启用，返回 true）。
auto TableDataPage::VerifyChecksum() const -> bool {
    if (header_.checksum == 0) return true;
    const uint32_t end = header_.null_bitmap_offset +
                         static_cast<uint32_t>(((header_.num_rows + 7) / 8) * header_.num_columns);
    return Fnv1a(page_->data, K_HEADER_SIZE, end) == header_.checksum;
}


auto TableDataPage::GetColumnRaw(size_t col_idx, std::vector<char> &out) const -> size_t {
    const ColDirEntry &entry = cols_.at(col_idx);
    return PageCodec::Decompress(static_cast<CompressionType>(header_.compression),
                                 page_->data + entry.data_offset, entry.compressed_size,
                                 entry.raw_size, out);
}


auto TableDataPage::GetColumnValidity(size_t col_idx, std::vector<uint8_t> &out) const -> void {
    const size_t bitmap_bytes = (header_.num_rows + 7) / 8;
    out.resize(bitmap_bytes);
    const char *src = page_->data + header_.null_bitmap_offset + col_idx * bitmap_bytes;
    memcpy(out.data(), src, bitmap_bytes);
}


auto TableDataPage::FreeSpace() const -> size_t {
    return 0;
}
