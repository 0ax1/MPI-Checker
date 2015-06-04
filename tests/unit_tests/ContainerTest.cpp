#include "gtest/gtest.h"
#include <vector>
#include "Container.hpp"

TEST(Container, isContained) {
    std::vector<int> v{0, 1, 2, 3, 4, 5, 6, 8, 9};
    EXPECT_TRUE(cont::isContained(v, 0));
    EXPECT_TRUE(cont::isContained(v, 3));
    EXPECT_TRUE(cont::isContained(v, 9));
}

TEST(Container, isNotContained) {
    std::vector<int> v{1, 2, 4, 5, 6, 8};
    EXPECT_FALSE(cont::isContained(v, 0));
    EXPECT_FALSE(cont::isContained(v, 3));
    EXPECT_FALSE(cont::isContained(v, 9));
}

TEST(Container, isContainedPred) {
    std::vector<int> v{0, 1, 2, 3, 4, 5, 6, 8, 9};
    EXPECT_TRUE(cont::isContainedPred(v, [](int x) { return x == 0; }));
    EXPECT_TRUE(cont::isContainedPred(v, [](int x) { return x == 3; }));
    EXPECT_TRUE(cont::isContainedPred(v, [](int x) { return x == 9; }));
}

TEST(Container, isNotContainedPred) {
    std::vector<int> v{1, 2, 4, 5, 6, 8};
    EXPECT_FALSE(cont::isContainedPred(v, [](int x) { return x == 0; }));
    EXPECT_FALSE(cont::isContainedPred(v, [](int x) { return x == 3; }));
    EXPECT_FALSE(cont::isContainedPred(v, [](int x) { return x == 9; }));
}

TEST(Container, erase) {
    std::vector<int> v{0, 1, 2, 3, 4, 5, 6, 8, 9};
    int x = 0, y = 3, z = 9;

    cont::erase(v, x);
    std::vector<int> comp{1, 2, 3, 4, 5, 6, 8, 9};
    EXPECT_EQ(v, comp);

    cont::erase(v, y);
    comp = {1, 2, 4, 5, 6, 8, 9};
    EXPECT_EQ(v, comp);

    cont::erase(v, z);
    comp = {1, 2, 4, 5, 6, 8};
    EXPECT_EQ(v, comp);

    std::vector<int> v2{0, 1, 1, 3, 4, 5, 6, 8, 9};
    std::vector<int> comp2{0, 1, 3, 4, 5, 6, 8, 9};
    int x2 = 1;
    cont::erase(v2, x2);
    EXPECT_EQ(v2, comp2);
}

TEST(Container, eraseAll) {
    std::vector<int> v{1, 1, 2, 3, 1, 5, 6, 8, 9};
    int x = 1;
    cont::eraseAll(v, x);

    EXPECT_FALSE(cont::isContained(v, x));

    std::vector<int> comp{2, 3, 5, 6, 8, 9};
    EXPECT_EQ(v, comp);
}

TEST(Container, erasePred) {
    std::vector<int> v{1, 1, 2, 3, 1, 5, 6, 8, 9};
    cont::erasePred(v, [](int x) { return x == 1; });
    std::vector<int> comp{1, 2, 3, 1, 5, 6, 8, 9};
    EXPECT_EQ(v, comp);

    cont::erasePred(v, [](int x) { return x == 1; });
    comp = {2, 3, 1, 5, 6, 8, 9};
    EXPECT_EQ(v, comp);

    cont::erasePred(v, [](int x) { return x == 9; });
    comp = {2, 3, 1, 5, 6, 8};
    EXPECT_EQ(v, comp);
}

TEST(Container, eraseIndex) {
    std::vector<int> v{0, 1, 2, 3};
    cont::eraseIndex(v, 1);
    std::vector<int> comp{0, 2, 3};
    EXPECT_EQ(v, comp);
}

TEST(Container, copy) {
    std::vector<int> v{0, 1};
    std::vector<int> v2{2, 3};
    std::vector<int> comp{0, 1, 2, 3};
    cont::copy(v2, v);
    EXPECT_EQ(v, comp);
}


TEST(Container, isPermutation) {
    std::vector<int> v1{0, 1, 2, 3};
    std::vector<int> v2{1, 0, 3, 2};

    EXPECT_TRUE(cont::isPermutation(v1, v2));

    std::vector<int> v3{1, 0, 4, 2};
    EXPECT_FALSE(cont::isPermutation(v1, v3));
}
