//
// Created by huan.yang on 2026-05-09.
//
#pragma once
#include <memory>
#include <unordered_map>
#include <vector>

#include "catalog/catalog.h"
#include "buffer/table_scan_iterator.h"
#include "buffer/table_heap.h"
#include "planner/physical/physical_operator.h"
#include "binder/expression/bound_expression.h"
#include "index/index_key.h"

namespace chickenDB {
    // 索引扫描：通过 catalog 中已注册的索引定位匹配行（点查或闭区间范围查），再按 RID
    // 回表取行。当未绑定可用索引时，退化为全表扫描 + 谓词过滤（保证语义正确）。
    //
    // planner 在谓词形如 col = v / col BETWEEN lo AND hi 且该列上有索引时，设置
    // index_name_ 与 lookup_lo_/lookup_hi_ 走索引路径；否则只给 predicate_ 走全扫。
    class PhysicalIndexScan : public PhysicalOperator {
    public:
        explicit PhysicalIndexScan(table_id_t table_id, std::shared_ptr<Catalog> catalog,
                                   std::unique_ptr<BoundExpression> predicate)
            : PhysicalOperator(PhysicalOperatorType::IndexScan),
              table_id_(table_id), catalog_(std::move(catalog)), predicate_(std::move(predicate)) {}
        ~PhysicalIndexScan() override = default;

        auto Init() -> void override;
        auto Next() -> Chunk * override;
        auto Close() -> void override;

        table_id_t table_id_;
        std::shared_ptr<Catalog> catalog_;
        std::unique_ptr<BoundExpression> predicate_; // 全扫回退时的过滤条件（可空）

        // 索引路径参数（planner 设置）：用名字绑定索引 + 查找区间 [lo, hi]（点查则 lo==hi）。
        std::string index_name_;        // 空 = 不走索引，退化全扫
        IndexKey lookup_lo_;
        IndexKey lookup_hi_;
        bool is_range_{false};          // false = 点查（用 lookup_lo_）

    private:
        // 索引路径状态。
        bool use_index_{false};
        std::unique_ptr<TableHeap> heap_;
        std::vector<RID> rids_;
        bool emitted_{false};
        Chunk output_;

        // 全扫回退路径状态。
        std::unique_ptr<TableScanIterator> it_;
        Chunk scan_chunk_;
        std::unordered_map<col_id_t, size_t> col_map_;
    };
}
