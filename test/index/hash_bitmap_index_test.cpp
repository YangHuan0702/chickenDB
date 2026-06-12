//
// Created by huan.yang on 2026-06-11.
//
#include <memory>
#include <vector>

#include "gtest/gtest.h"

#include "index/hash_index.h"
#include "index/bitmap_index.h"
#include "common/chicken_execption.h"

using namespace chickenDB;

namespace {
    auto K(double v) -> IndexKey { return IndexKey(std::vector<double>{v}); }
}

TEST(HashIndex, PointLookupAndErase) {
    HashIndex idx;
    idx.Insert(K(1), RID(1, 0));
    idx.Insert(K(1), RID(1, 1)); // 非唯一
    idx.Insert(K(2), RID(2, 0));

    EXPECT_EQ(idx.Type(), IndexType::Hash);
    EXPECT_FALSE(idx.SupportsRange());
    EXPECT_EQ(idx.Find(K(1)).size(), 2U);
    EXPECT_EQ(idx.Find(K(2)).size(), 1U);
    EXPECT_TRUE(idx.Find(K(99)).empty());

    EXPECT_TRUE(idx.Erase(K(1), RID(1, 0)));
    EXPECT_EQ(idx.Find(K(1)).size(), 1U);
}

TEST(HashIndex, RangeThrows) {
    HashIndex idx;
    idx.Insert(K(1), RID(1, 0));
    EXPECT_THROW(idx.Range(K(0), K(9)), ChickenException);
}

TEST(BitmapIndex, PointAndRange) {
    BitmapIndex idx;
    // 低基数：键 0/1/2，多行命中。
    idx.Insert(K(0), RID(0, 0));
    idx.Insert(K(1), RID(0, 1));
    idx.Insert(K(1), RID(0, 2));
    idx.Insert(K(2), RID(0, 3));

    EXPECT_EQ(idx.Type(), IndexType::Bitmap);
    EXPECT_TRUE(idx.SupportsRange());
    EXPECT_EQ(idx.Find(K(1)).size(), 2U);

    // 范围 [0,1] -> 命中 key0(1行) + key1(2行) = 3。
    EXPECT_EQ(idx.Range(K(0), K(1)).size(), 3U);
    // 范围 [1,2] -> key1(2) + key2(1) = 3。
    EXPECT_EQ(idx.Range(K(1), K(2)).size(), 3U);
}

TEST(BitmapIndex, EraseRemovesEmptyBucket) {
    BitmapIndex idx;
    idx.Insert(K(5), RID(1, 0));
    EXPECT_TRUE(idx.Erase(K(5), RID(1, 0)));
    EXPECT_TRUE(idx.Find(K(5)).empty());
    EXPECT_FALSE(idx.Erase(K(5), RID(1, 0)));
}

// 通过抽象接口多态使用三种索引（能力查询区分行为）。
TEST(Index, PolymorphicCapability) {
    std::vector<std::unique_ptr<Index>> idxs;
    idxs.push_back(std::make_unique<HashIndex>());
    idxs.push_back(std::make_unique<BitmapIndex>());

    for (auto &idx : idxs) {
        idx->Insert(K(1), RID(1, 0));
        EXPECT_EQ(idx->Find(K(1)).size(), 1U);
        if (idx->SupportsRange()) {
            EXPECT_NO_THROW(idx->Range(K(0), K(2)));
        } else {
            EXPECT_THROW(idx->Range(K(0), K(2)), ChickenException);
        }
    }
}
