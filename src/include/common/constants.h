//
// Created by huan.yang on 2026-01-27.
//
#pragma once
#include <cstdint>


namespace chickenDB {
#define DEFAULT_SCHEMA "main"
    //! a saner size_t for loop indices etc
    typedef uint64_t index_t;

    //! The type used for row identifiers
    typedef int64_t row_t;

    //! The value used to signify an invalid index entry
    extern const index_t INVALID_INDEX;

    typedef uint8_t data_t;
    typedef data_t *data_ptr_t;
    typedef const data_t *const_data_ptr_t;
}
