//
// Created by 杨欢 on 2026/5/6.
//
#include "common/enum/statement_type.h"

#include "common/chicken_execption.h"

using namespace chickenDB;


static auto ConversionChickenDBColumnType(hsql::ColumnType column_type) -> ColumnType {
    switch (column_type.data_type) {
        case hsql::DataType::VARCHAR: return ColumnType::VARCHAR;
        case hsql::DataType::INT:
        case hsql::DataType::BIGINT:
        case hsql::DataType::SMALLINT:
        case hsql::DataType::LONG:
            return ColumnType::NUMBER;
        case hsql::DataType::DECIMAL:
        case hsql::DataType::FLOAT:
        case hsql::DataType::DOUBLE:
            return ColumnType::DOUBLE;
        default: throw ChickenException("unknown columnType ");
    }
}
