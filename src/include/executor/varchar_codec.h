//
// Created by huan.yang on 2026-06-16.
//
#pragma once
#include <cstdint>
#include <cstring>
#include <vector>

#include "common/chicken_execption.h"
#include "executor/chunk.h"

namespace chickenDB {
    // 变长列的自描述序列化编解码。页层（TableDataPage）把列数据当不透明字节流压缩，
    // 故变长列必须把行边界信息打进 raw buffer 自身。Arrow 风格扁平布局：
    //
    //   [ uint32 num_rows ][ int32 offsets[num_rows+1] ][ char data[ offsets[num_rows] ] ]
    //
    // 其中 offsets[0]==0，offsets[r+1]-offsets[r] 为第 r 行字节长度，offsets[num_rows]
    // 为数据区总字节数。NULL 行长度为 0（validity 单独走 page 的 null bitmap）。
    class VarcharCodec {
    public:
        // 把 vec 的 [row_start, row_start+cnt) 行序列化为自描述 buffer。
        // offsets 从 0 重排，使切片独立自洽（多页切分时每页一段）。
        static auto SerializeVarlenColumn(const Vector &vec, size_t row_start, size_t cnt)
            -> std::vector<char> {
            const uint32_t n = static_cast<uint32_t>(cnt);
            std::vector<int32_t> local_offsets(n + 1, 0);
            // 先累计每行长度得到本地 offsets，并算出数据区总长。
            for (uint32_t i = 0; i < n; i++) {
                const std::string_view s = vec.GetString(row_start + i);
                local_offsets[i + 1] = local_offsets[i] + static_cast<int32_t>(s.size());
            }
            const size_t header_bytes = sizeof(uint32_t) + (n + 1) * sizeof(int32_t);
            const size_t data_bytes = static_cast<size_t>(local_offsets[n]);
            std::vector<char> out(header_bytes + data_bytes);

            size_t cursor = 0;
            std::memcpy(out.data() + cursor, &n, sizeof(uint32_t));
            cursor += sizeof(uint32_t);
            std::memcpy(out.data() + cursor, local_offsets.data(), (n + 1) * sizeof(int32_t));
            cursor += (n + 1) * sizeof(int32_t);
            for (uint32_t i = 0; i < n; i++) {
                const std::string_view s = vec.GetString(row_start + i);
                if (!s.empty()) {
                    std::memcpy(out.data() + cursor, s.data(), s.size());
                    cursor += s.size();
                }
            }
            return out;
        }

        // 反序列化自描述 buffer 到一个已 Init(VARCHAR, >=num_rows) 的 Vector。
        // 仅填充变长数据；validity 由调用方另行设置（来自 page null bitmap）。
        static auto DeserializeVarlenColumn(const char *raw, size_t raw_size, Vector &out) -> void {
            ChickenException::AssertCondition(raw_size >= sizeof(uint32_t),
                                              "[VarcharCodec] raw buffer too small for header");
            size_t cursor = 0;
            uint32_t n = 0;
            std::memcpy(&n, raw + cursor, sizeof(uint32_t));
            cursor += sizeof(uint32_t);

            const size_t off_bytes = (static_cast<size_t>(n) + 1) * sizeof(int32_t);
            ChickenException::AssertCondition(raw_size >= sizeof(uint32_t) + off_bytes,
                                              "[VarcharCodec] raw buffer too small for offsets");
            // 拷进对齐的数组：raw 来自 vector<char>，直接 reinterpret_cast 到 int32_t*
            // 不保证对齐（UB）。memcpy 既安全又零 -Werror 噪声。
            std::vector<int32_t> offsets(n + 1);
            std::memcpy(offsets.data(), raw + cursor, off_bytes);
            cursor += off_bytes;

            const size_t data_len = static_cast<size_t>(offsets[n]);
            ChickenException::AssertCondition(raw_size >= cursor + data_len,
                                              "[VarcharCodec] raw buffer too small for data");
            out.SetVarColumn(offsets.data(), n, raw + cursor, data_len);
        }
    };
}
