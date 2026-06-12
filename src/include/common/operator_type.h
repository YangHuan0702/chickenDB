//
// Created by huan.yang on 2026-05-09.
//
#pragma once

namespace chickenDB {
    enum class LogicalOperatorType {
        SCAN,
        FILTER,
        PROJECT,
        JOIN,
        AGGREGATE,
        SORT,
        LIMIT,
        CREATEA_TABLE,
        DELETE,
        UPDATE,
        INSERT,
        CREATE_INDEX,
    };

    enum class PhysicalOperatorType {
        SeqScan,
        IndexScan,
        IndexOnlyScan,
        BitmapScan,

        NestedLoopJoin,
        HashJoin,
        MergeJoin,
        IndexNLJoin,

        HashAggregate,
        StreamAggregate,

        InMemorySort,
        ExternalSort,
        TopN,

        Filter,
        Project,
        Limit,
        Distinct,

        CREATE_TABLE,
        INSERT,
        CREATE_INDEX,
        DELETE,
        UPDATE,
    };
}
