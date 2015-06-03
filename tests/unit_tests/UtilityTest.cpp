#include "gtest/gtest.h"
#include "Utility.hpp"


TEST(Utility, split) {
    auto s = util::split("aaa:bbb", ':');
    EXPECT_EQ(s[0], "aaa");
    EXPECT_EQ(s[1], "bbb");
}
