#include "gtest/gtest.h"
#include "Utility.hpp"

TEST(Utility, split) {
    auto s = util::split("aaa:bbb", ':');
    EXPECT_EQ(s[0], "aaa");
    EXPECT_EQ(s[1], "bbb");

    auto s2 = util::split("aaa,bbb", ',');
    EXPECT_EQ(s2[0], "aaa");
    EXPECT_EQ(s2[1], "bbb");
}
