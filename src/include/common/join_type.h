//
// Created by huan.yang on 2026-05-09.
//
#pragma once

namespace chickenDB {

    enum class JoinType {
        INNER,
        LEFT,
        RIGHT,
        FULL,
        CROSS,
        SEMI,
        ANTI,
        MARK,
        SINGLE    // 标量子查询，保证右侧最多一行
    };

}
