#include <itertools.hpp>
#include <cassert>
#include <iostream>
#include <unordered_map>
using namespace itertools;

#define EXPECT_EQ(A, B)                                             \
    if (!((A) == (B)))                                              \
    {                                                               \
        std::cerr << "Expect eq: " << #A << "=" << #B << " failed"; \
        std::abort();                                               \
    }

int main()
{
    std::vector<int> fff = {1, 2, 3, 4, 5, 6};
    const auto &fff_c = fff;
    auto out = iter_on(fff)
                   .map([](int a) { return a + 1; })
                   .filter_inplace([](int v) { return v <= 4; })
                   .get();
    EXPECT_EQ(out, (std::vector<int>{2, 3, 4}));
    EXPECT_EQ(fff, (std::vector<int>{1, 2, 3, 4, 5, 6}));

    auto out2 = iter_on(fff_c).filter([](int a) { return a < 3; }).get();
    EXPECT_EQ(out2, (std::vector<int>{1, 2}));

    std::vector<int> fff2 = {1, 3, 4, 5, 6};
    std::vector<int> fff3 = {2, 5, 6, 7, 8};

    auto mul = iter_on(zip(fff2, fff3))
                   .map([](std::pair<int &, int &> v) {
                       return v.first * v.second;
                   })
                   .get();
    EXPECT_EQ(mul, (std::vector<int>{2, 15, 24, 35, 48}));

    iter_on(zip(fff2, fff3)).map_inplace([](std::pair<int &, int &> v) {
        return std::pair<int, int>(v.first + 1, v.second + 1);
    });

    EXPECT_EQ(fff2, (std::vector<int>{2, 4, 5, 6, 7}));
    EXPECT_EQ(fff3, (std::vector<int>{3, 6, 7, 8, 9}));

    fff3 = {2, 5, 6, 7, 8};
    iter_on(zip(fff3, range(5))).map_inplace([](std::pair<int &, int> v) {
        return std::pair<int, int>(v.first + v.second, v.second);
    });
    EXPECT_EQ(fff3, (std::vector<int>{2, 6, 8, 10, 12}));

    fff3 = {1, 2, 3};
    auto sum = iter_on(fff3).reduce(10, [](int v1, int v2) { return v1 + v2; });
    EXPECT_EQ(sum, 16);

    auto index = iter_on(zip(fff3, range(3)))
                     .find_if([](std::pair<int &, int> v) {
                         return v.first == 2;
                     })
                     .second;
    EXPECT_EQ(index, 1);
    auto outmap = iter_on(zip(fff3, range(3))).as<std::unordered_map<int, int>>();
    EXPECT_EQ(outmap, (std::unordered_map<int, int>{{1, 0}, {2, 1}, {3, 2}}));
    std::cerr<<"All tests done\n";
}