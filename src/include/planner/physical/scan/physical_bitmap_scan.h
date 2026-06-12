//
// Created by huan.yang on 2026-05-09.
//
#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "catalog/catalog.h"
#include "buffer/table_scan_iterator.h"
#include "buffer/table_heap.h"
#include "planner/physical/physical_operator.h"
#include "binder/expression/bound_expression.h"
#include "index/index_key.h"

namespace chickenDB {
    // 位图扫描：用 bitmap 索引按键定位命中行的 RID 集合（点查或范围查），再按 RID
    // 批量回表读取。未绑定可用索引时退化为全表扫描 + 谓词过滤（语义正确）。
    class PhysicalBitmapScan : public PhysicalOperator {
    public:
        explicit PhysicalBitmapScan(table_id_t table_id, std::shared_ptr<Catalog> catalog,
                                    std::unique_ptr<BoundExpression> predicate)
            : PhysicalOperator(PhysicalOperatorType::BitmapScan),
              table_id_(table_id), catalog_(std::move(catalog)), predicate_(std::move(predicate)) {}
        ~PhysicalBitmapScan() override = default;

        auto Init() -> void override;
        auto Next() -> Chunk * override;
        auto Close() -> void override;

        table_id_t table_id_;
        std::shared_ptr<Catalog> catalog_;
        std::unique_ptr<BoundExpression> predicate_;

        // 索引路径参数（planner 设置）：绑定 bitmap 索引 + 查找区间 [lo,hi]（点查 lo==hi）。
        std::string index_name_;
        IndexKey lookup_lo_;
        IndexKey lookup_hi_;
        bool is_range_{false};

    private:
        // 索引路径状态。
        bool use_index_{false};
        std::unique_ptr<TableHeap> heap_;
        std::vector<RID> rids_;
        bool emitted_{false};
        Chunk output_;
        // 全扫回退状态。
        std::unique_ptr<TableScanIterator> it_;
        Chunk scan_chunk_;
        std::unordered_map<col_id_t, size_t> col_map_;
    };
}
