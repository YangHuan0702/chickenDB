//
// 阶段3：LikeMatcher 纯函数单测（%/_/边界/ILIKE）。
//
#include "gtest/gtest.h"

#include "executor/like_matcher.h"

using namespace chickenDB;

TEST(LikeMatcher, LiteralMatch) {
    EXPECT_TRUE(LikeMatcher::Match("hello", "hello"));
    EXPECT_FALSE(LikeMatcher::Match("hello", "world"));
    EXPECT_FALSE(LikeMatcher::Match("hell", "hello"));
}

TEST(LikeMatcher, PercentWildcard) {
    EXPECT_TRUE(LikeMatcher::Match("hello", "h%"));
    EXPECT_TRUE(LikeMatcher::Match("hello", "%o"));
    EXPECT_TRUE(LikeMatcher::Match("hello", "h%o"));
    EXPECT_TRUE(LikeMatcher::Match("hello", "%"));
    EXPECT_TRUE(LikeMatcher::Match("", "%"));        // % 匹配空串
    EXPECT_TRUE(LikeMatcher::Match("hello", "%ell%"));
    EXPECT_FALSE(LikeMatcher::Match("hello", "h%x"));
    EXPECT_TRUE(LikeMatcher::Match("aaa", "%a%a%")); // 回溯
}

TEST(LikeMatcher, UnderscoreWildcard) {
    EXPECT_TRUE(LikeMatcher::Match("hello", "h_llo"));
    EXPECT_TRUE(LikeMatcher::Match("hello", "_____"));
    EXPECT_FALSE(LikeMatcher::Match("hello", "____"));  // 长度不符
    EXPECT_FALSE(LikeMatcher::Match("hello", "______"));
    EXPECT_TRUE(LikeMatcher::Match("hello", "h_l_o"));
}

TEST(LikeMatcher, MixedWildcards) {
    EXPECT_TRUE(LikeMatcher::Match("abcdef", "a%d_f"));
    EXPECT_TRUE(LikeMatcher::Match("abcdef", "_%f"));
    EXPECT_FALSE(LikeMatcher::Match("abcdef", "a%z_f"));
}

TEST(LikeMatcher, CaseInsensitiveILike) {
    EXPECT_TRUE(LikeMatcher::Match("Hello", "hello", /*ci=*/true));
    EXPECT_TRUE(LikeMatcher::Match("HELLO", "h%o", true));
    EXPECT_FALSE(LikeMatcher::Match("Hello", "hello", /*ci=*/false));
}

TEST(LikeMatcher, EmptyPattern) {
    EXPECT_TRUE(LikeMatcher::Match("", ""));
    EXPECT_FALSE(LikeMatcher::Match("x", ""));
}
