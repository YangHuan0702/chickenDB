//
// 阶段0：类型基座单测。验证 IsVarlen 判定与 TypeSize 对变长/定长类型的返回值。
//
#include "gtest/gtest.h"

#include "common/enum/statement_type.h"

using namespace chickenDB;

TEST(TypeTest, IsVarlenClassification) {
    EXPECT_TRUE(IsVarlen(ColumnType::VARCHAR));
    EXPECT_TRUE(IsVarlen(ColumnType::VARCHAR2));
    EXPECT_FALSE(IsVarlen(ColumnType::NUMBER));
    EXPECT_FALSE(IsVarlen(ColumnType::DOUBLE));
}

TEST(TypeTest, FixedTypeSizes) {
    EXPECT_EQ(TypeSizeConversion::TypeSize(ColumnType::NUMBER), 4u);
    EXPECT_EQ(TypeSizeConversion::TypeSize(ColumnType::DOUBLE), 8u);
}

TEST(TypeTest, VarlenTypeSizeIsZero) {
    // 变长类型不能按固定跨步寻址，TypeSize 返回 0 作为信号。
    EXPECT_EQ(TypeSizeConversion::TypeSize(ColumnType::VARCHAR), 0u);
    EXPECT_EQ(TypeSizeConversion::TypeSize(ColumnType::VARCHAR2), 0u);
}
