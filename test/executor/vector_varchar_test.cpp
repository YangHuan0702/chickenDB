//
// 阶段1：Vector 变长存储 + VarcharCodec 编解码单测。
//
#include <string>
#include <string_view>
#include <vector>

#include "gtest/gtest.h"

#include "executor/chunk.h"
#include "executor/varchar_codec.h"

using namespace chickenDB;

TEST(VectorVarcharTest, AppendAndGet) {
    Vector v;
    v.Init(ColumnType::VARCHAR, 4);
    EXPECT_TRUE(v.IsVar());
    v.AppendString("hello");
    v.AppendString("");
    v.AppendString("world!!");
    EXPECT_EQ(v.VarRowCount(), 3u);
    EXPECT_EQ(v.GetString(0), std::string_view("hello"));
    EXPECT_EQ(v.GetString(1), std::string_view(""));
    EXPECT_EQ(v.GetString(2), std::string_view("world!!"));
}

TEST(VectorVarcharTest, EmbeddedNulBytes) {
    Vector v;
    v.Init(ColumnType::VARCHAR, 2);
    std::string s("a\0b\0c", 5); // 含内嵌 \0
    v.AppendString(s);
    v.AppendString("tail");
    EXPECT_EQ(v.GetString(0).size(), 5u);
    EXPECT_EQ(std::string(v.GetString(0)), s);
    EXPECT_EQ(v.GetString(1), std::string_view("tail"));
}

TEST(VectorVarcharTest, SerializeRoundTrip) {
    Vector v;
    v.Init(ColumnType::VARCHAR, 5);
    const std::vector<std::string> data = {"apple", "", "banana", "c", "delta"};
    for (const auto &s : data) v.AppendString(s);

    auto buf = VarcharCodec::SerializeVarlenColumn(v, 0, data.size());

    Vector out;
    out.Init(ColumnType::VARCHAR, 5);
    VarcharCodec::DeserializeVarlenColumn(buf.data(), buf.size(), out);
    ASSERT_EQ(out.VarRowCount(), data.size());
    for (size_t i = 0; i < data.size(); i++) {
        EXPECT_EQ(std::string(out.GetString(i)), data[i]);
    }
}

TEST(VectorVarcharTest, SerializeSliceReindexesOffsets) {
    // 切片 [2,5) 应独立自洽（offsets 从 0 重排）。
    Vector v;
    v.Init(ColumnType::VARCHAR, 6);
    const std::vector<std::string> data = {"zero", "one", "two", "three", "four", "five"};
    for (const auto &s : data) v.AppendString(s);

    auto buf = VarcharCodec::SerializeVarlenColumn(v, 2, 3); // two,three,four
    Vector out;
    out.Init(ColumnType::VARCHAR, 3);
    VarcharCodec::DeserializeVarlenColumn(buf.data(), buf.size(), out);
    ASSERT_EQ(out.VarRowCount(), 3u);
    EXPECT_EQ(std::string(out.GetString(0)), "two");
    EXPECT_EQ(std::string(out.GetString(1)), "three");
    EXPECT_EQ(std::string(out.GetString(2)), "four");
}

TEST(VectorVarcharTest, FixedWidthPathUnaffected) {
    // 定长列回归：NUMBER/DOUBLE 仍走 GetValue/SetValue，IsVar 为 false。
    Vector vn;
    vn.Init(ColumnType::NUMBER, 3);
    EXPECT_FALSE(vn.IsVar());
    vn.SetValue<int32_t>(0, 42);
    vn.SetValue<int32_t>(1, -7);
    EXPECT_EQ(vn.GetValue<int32_t>(0), 42);
    EXPECT_EQ(vn.GetValue<int32_t>(1), -7);

    Vector vd;
    vd.Init(ColumnType::DOUBLE, 2);
    EXPECT_FALSE(vd.IsVar());
    vd.SetValue<double>(0, 3.5);
    EXPECT_DOUBLE_EQ(vd.GetValue<double>(0), 3.5);
}
