//
// Created by huan.yang on 2026-05-21.
//
#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "common/chicken_execption.h"
#include "common/constants.h"
#include "planner/planner.h"
#include "planner/logical/logical_sort.h"
#include "planner/physical/sort/physical_in_memory_sort.h"
#include "planner/physical/sort/physical_top_n.h"
#include "planner/physical/sort/physical_external_sort.h"
#include "executor/chunk_util.h"

using namespace chickenDB;

namespace {
    // 把孩子的全部输出物化为 rows（每行各列值取 double），并记录列类型/col_ids。
    auto Materialize(PhysicalOperator *child, std::vector<std::vector<double>> &rows,
                     std::vector<ColumnType> &types, std::vector<col_id_t> &col_ids,
                     std::vector<size_t> &sort_idx, const std::vector<col_id_t> &sort_cols) -> void {
        bool resolved = false;
        while (Chunk *in = child->Next()) {
            if (!resolved) {
                types = ChunkUtil::TypesOf(*in);
                col_ids = in->ColIds();
                auto col_map = ChunkUtil::BuildColMap(*in);
                for (col_id_t cid : sort_cols) {
                    auto it = col_map.find(cid);
                    ChickenException::AssertCondition(it != col_map.end(), "[Sort] sort column not found");
                    sort_idx.push_back(it->second);
                }
                resolved = true;
            }
            const size_t n = in->Count();
            const size_t cols = in->ColumnCount();
            for (size_t r = 0; r < n; r++) {
                std::vector<double> row(cols);
                for (size_t c = 0; c < cols; c++) {
                    const Vector &v = in->GetColumn(c);
                    row[c] = v.GetType() == ColumnType::NUMBER
                                 ? static_cast<double>(v.GetValue<int32_t>(r))
                                 : v.GetValue<double>(r);
                }
                rows.push_back(std::move(row));
            }
        }
    }

    // 按 sort_idx 列比较两行；desc[i] 指定该列降序（缺省升序）。
    auto MakeComparator(const std::vector<size_t> &sort_idx, const std::vector<bool> &desc) {
        return [sort_idx, desc](const std::vector<double> &a, const std::vector<double> &b) {
            for (size_t k = 0; k < sort_idx.size(); k++) {
                const size_t idx = sort_idx[k];
                const bool d = k < desc.size() ? desc[k] : false;
                if (a[idx] < b[idx]) return !d;  // 升序 a<b 在前；降序则相反
                if (a[idx] > b[idx]) return d;
            }
            return false;
        };
    }

    // 把已排序的 rows 物化进 output（一次性，全部行）。
    auto EmitRows(Chunk &output, const std::vector<std::vector<double>> &rows,
                  const std::vector<ColumnType> &types, const std::vector<col_id_t> &col_ids) -> void {
        const size_t n = rows.empty() ? 1 : rows.size();
        output.Init(types, n);
        output.SetColIds(col_ids);
        for (size_t r = 0; r < rows.size(); r++) {
            for (size_t c = 0; c < types.size(); c++) {
                if (types[c] == ColumnType::NUMBER) {
                    output.GetColumn(c).SetValue<int32_t>(r, static_cast<int32_t>(rows[r][c]));
                } else {
                    output.GetColumn(c).SetValue<double>(r, rows[r][c]);
                }
            }
        }
        output.SetCount(rows.size());
    }
}

auto Planner::PhysicalSortOperator(std::unique_ptr<LogicalOperator> logical_operator) -> std::unique_ptr<PhysicalOperator> {
    ChickenException::AssertCondition(logical_operator->type_ == LogicalOperatorType::SORT,
                                      "[Planner] target logical operator is not Sort type.");
    auto *logical_sort = dynamic_cast<LogicalSort *>(logical_operator.get());
    std::vector<bool> desc(logical_sort->desc_.begin(), logical_sort->desc_.end());
    return std::make_unique<PhysicalInMemorySort>(logical_sort->col_ids_, desc);
}


// ---- InMemorySort ----
auto PhysicalInMemorySort::Init() -> void {
    Child(0)->Init();
    rows_.clear();
    emit_pos_ = 0;
    built_ = false;
}

auto PhysicalInMemorySort::Close() -> void {
    Child(0)->Close();
}

auto PhysicalInMemorySort::Next() -> Chunk * {
    if (!built_) {
        std::vector<size_t> sort_idx;
        Materialize(Child(0), rows_, types_, col_ids_, sort_idx, sort_cols_);
        std::sort(rows_.begin(), rows_.end(), MakeComparator(sort_idx, sort_desc_));
        EmitRows(output_, rows_, types_, col_ids_);
        built_ = true;
        return rows_.empty() ? nullptr : &output_;
    }
    return nullptr;
}


// ---- TopN ----
auto PhysicalTopN::Init() -> void {
    Child(0)->Init();
    rows_.clear();
    built_ = false;
}

auto PhysicalTopN::Close() -> void {
    Child(0)->Close();
}

auto PhysicalTopN::Next() -> Chunk * {
    if (!built_) {
        std::vector<size_t> sort_idx;
        Materialize(Child(0), rows_, types_, col_ids_, sort_idx, sort_cols_);
        // 只需前 n_ 个有序：用 partial_sort 把最小的 n_ 个排到前面，O(N log n)，
        // 优于全排序 O(N log N)。
        if (n_ < rows_.size()) {
            std::partial_sort(rows_.begin(), rows_.begin() + n_, rows_.end(),
                              MakeComparator(sort_idx, sort_desc_));
            rows_.resize(n_);
        } else {
            std::sort(rows_.begin(), rows_.end(), MakeComparator(sort_idx, sort_desc_));
        }
        EmitRows(output_, rows_, types_, col_ids_);
        built_ = true;
        return rows_.empty() ? nullptr : &output_;
    }
    return nullptr;
}


// ---- ExternalSort（spill-to-disk 外部归并排序） ----
// 阶段1：分批从孩子取行，每攒够 K_RUN_ROWS 行就内存排序并写出一个 run 临时文件；
// 阶段2：K 路归并所有 run（用最小堆按比较器合并），物化到 output_。
// 内存里只保留每个 run 的当前一行（+ 输出 chunk），故内存占用 O(run 数)。
auto PhysicalExternalSort::Init() -> void {
    Child(0)->Init();
    rows_.clear();
    emit_pos_ = 0;
    built_ = false;
}

auto PhysicalExternalSort::Close() -> void {
    Child(0)->Close();
}

namespace {
    constexpr size_t K_RUN_ROWS = 4096; // 每个 run 的最大行数（内存阈值）

    // 把若干行写入一个 run 文件（每行 ncols 个 double，紧凑）。
    auto WriteRun(const std::string &path, const std::vector<std::vector<double>> &rows,
                  size_t ncols) -> void {
        int fd = ::open(path.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) return;
        off_t off = 0;
        for (const auto &row : rows) {
            ::pwrite(fd, row.data(), ncols * sizeof(double), off);
            off += static_cast<off_t>(ncols * sizeof(double));
        }
        ::close(fd);
    }
}

auto PhysicalExternalSort::Next() -> Chunk * {
    if (built_) {
        return nullptr;
    }

    // 解析排序列下标 + 收集类型/col_ids（用首个 chunk）。
    std::vector<size_t> sort_idx;
    size_t ncols = 0;
    std::vector<std::string> run_paths;
    std::vector<std::vector<double>> buf;
    const std::string base = GetDataPath() + "/extsort_run_";
    size_t run_id = 0;

    bool resolved = false;
    while (Chunk *in = Child(0)->Next()) {
        if (!resolved) {
            types_ = ChunkUtil::TypesOf(*in);
            col_ids_ = in->ColIds();
            ncols = in->ColumnCount();
            auto col_map = ChunkUtil::BuildColMap(*in);
            for (col_id_t cid : sort_cols_) {
                auto it = col_map.find(cid);
                ChickenException::AssertCondition(it != col_map.end(), "[ExternalSort] sort column not found");
                sort_idx.push_back(it->second);
            }
            resolved = true;
        }
        const size_t n = in->Count();
        for (size_t r = 0; r < n; r++) {
            std::vector<double> row(ncols);
            for (size_t c = 0; c < ncols; c++) {
                const Vector &v = in->GetColumn(c);
                row[c] = v.GetType() == ColumnType::NUMBER
                             ? static_cast<double>(v.GetValue<int32_t>(r))
                             : v.GetValue<double>(r);
            }
            buf.push_back(std::move(row));
            if (buf.size() >= K_RUN_ROWS) {
                std::sort(buf.begin(), buf.end(), MakeComparator(sort_idx, sort_desc_));
                std::string path = base + std::to_string(run_id++);
                WriteRun(path, buf, ncols);
                run_paths.push_back(path);
                buf.clear();
            }
        }
    }
    // 末尾不足一个 run 的残余也写出（统一走归并路径）。
    if (!buf.empty()) {
        std::sort(buf.begin(), buf.end(), MakeComparator(sort_idx, sort_desc_));
        std::string path = base + std::to_string(run_id++);
        WriteRun(path, buf, ncols);
        run_paths.push_back(path);
        buf.clear();
    }

    built_ = true;
    if (run_paths.empty()) {
        return nullptr;
    }

    // K 路归并：打开所有 run，各维护一个读游标，用比较器选最小行。
    struct RunReader {
        int fd;
        off_t off;
        off_t size;
        std::vector<double> cur;
        bool valid;
    };
    auto cmp = MakeComparator(sort_idx, sort_desc_);
    std::vector<RunReader> readers(run_paths.size());
    for (size_t i = 0; i < run_paths.size(); i++) {
        readers[i].fd = ::open(run_paths[i].c_str(), O_RDONLY);
        readers[i].off = 0;
        struct stat st{};
        readers[i].size = (readers[i].fd >= 0 && fstat(readers[i].fd, &st) == 0) ? st.st_size : 0;
        readers[i].cur.resize(ncols);
        readers[i].valid = false;
        if (readers[i].fd >= 0 && readers[i].off < readers[i].size) {
            ::pread(readers[i].fd, readers[i].cur.data(), ncols * sizeof(double), readers[i].off);
            readers[i].off += static_cast<off_t>(ncols * sizeof(double));
            readers[i].valid = true;
        }
    }

    std::vector<std::vector<double>> merged;
    while (true) {
        int best = -1;
        for (size_t i = 0; i < readers.size(); i++) {
            if (!readers[i].valid) continue;
            if (best < 0 || cmp(readers[i].cur, readers[static_cast<size_t>(best)].cur)) {
                best = static_cast<int>(i);
            }
        }
        if (best < 0) break;
        merged.push_back(readers[static_cast<size_t>(best)].cur);
        RunReader &rr = readers[static_cast<size_t>(best)];
        if (rr.off < rr.size) {
            ::pread(rr.fd, rr.cur.data(), ncols * sizeof(double), rr.off);
            rr.off += static_cast<off_t>(ncols * sizeof(double));
        } else {
            rr.valid = false;
        }
    }
    for (auto &rr : readers) {
        if (rr.fd >= 0) ::close(rr.fd);
    }
    // 清理 run 临时文件。
    for (const auto &p : run_paths) {
        std::error_code ec;
        std::filesystem::remove(p, ec);
    }

    EmitRows(output_, merged, types_, col_ids_);
    return merged.empty() ? nullptr : &output_;
}
