#ifndef __PROGTEST__

#include <cassert>
#include <iomanip>
#include <cstdint>
#include <iostream>
#include <memory>
#include <limits>
#include <optional>
#include <algorithm>
#include <bitset>
#include <list>
#include <array>
#include <vector>
#include <deque>
#include <unordered_set>
#include <unordered_map>
#include <set>
#include <map>
#include <stack>
#include <queue>
#include <random>
#include <type_traits>

using Price = unsigned long long;
using Employee = size_t;
inline constexpr Employee NO_EMPLOYEE = -1;
using Gift = size_t;

#endif

#pragma GCC optimize("Ofast,no-stack-protector,unroll-loops,fast-math")
//#pragma GCC target("sse,sse2,sse3,ssse3,sse4,popcnt,abm,mmx,avx")
//#pragma GCC target("avx2")



alignas(16) char buf[450 << 20];

size_t buf_ind = sizeof buf;
template<class T> struct small {
    typedef T value_type;
    small() {}
    template<class U> small(const U&) {}
    T* allocate(size_t n) {
        buf_ind -= n * sizeof(T);
        buf_ind &= 0 - alignof(T);
        return (T*)(buf + buf_ind);
    }
    void deallocate(T*, size_t) {}
};

struct GiftS {
    size_t idx;
    Price price;

    bool operator<(const GiftS &other) const {
        return price < other.price;
    }
};

constexpr size_t MAGIC_CONST = 32;

std::vector<GiftS> sorted_gifts(
        const std::vector<Price> &gift_prices
) {
    const ssize_t G = static_cast<ssize_t>(std::min(MAGIC_CONST, gift_prices.size()));
    std::vector<GiftS> gift_pairs(gift_prices.size());

    size_t idx = 0;
    for(const auto &gift: gift_prices) {
        gift_pairs[idx] = {idx, gift};
        idx++;
    }

    std::nth_element(gift_pairs.begin(),
                     gift_pairs.begin() + G,
                     gift_pairs.end());

    gift_pairs.resize(G);
    std::sort(gift_pairs.begin(), gift_pairs.end());
    return gift_pairs;
}

template<typename Allocator>
std::pair<Price, std::vector<Gift>> more_gifts_case(
        const std::vector<Employee> &bosses,
        const std::vector<Price> &gifts,
        const std::vector<std::vector<Employee, Allocator>> &subordinates,
        const std::vector<Employee> &directors,
        const std::vector<Employee> &order
) {
    const auto sorted_g = sorted_gifts(gifts);
    const auto N = bosses.size();
    const auto G = sorted_g.size();

    // for each employee best and the second-best pair (idx into sorted_g, price for subtree)
    std::vector<GiftS> DT(2 * N);

    std::array<Price, MAGIC_CONST> node_p{};
    for(const auto &emp: order) {
        size_t max_color = 0;
        Price opt_price_sum = 0;

        auto sub_begin = subordinates[emp].begin();
        const auto sub_end = subordinates[emp].end();
        if(sub_begin == sub_end) {
            DT[2 * emp] = {0, sorted_g[0].price};
            DT[2 * emp + 1] = {1, sorted_g[1].price};
            continue;
        }
        for(; sub_begin != sub_end; ++sub_begin) {
            const auto &sub = *sub_begin;
            opt_price_sum += DT[2 * sub].price;
            size_t gift_idx = DT[2 * sub].idx;

            max_color = std::max(max_color, gift_idx);
            node_p[DT[2*sub].idx] += DT[2*sub+1].price - DT[2*sub].price;
        }
        max_color = std::min(max_color + 3, G);

        size_t b_idx0 = 2 * emp;
        size_t b_idx1 = 2 * emp + 1;
        DT[b_idx0] = {0, node_p[0] + opt_price_sum + sorted_g[0].price};
        DT[b_idx1] = {1, node_p[1] + opt_price_sum + sorted_g[1].price};
        node_p[0] = node_p[1] = 0;

        if(DT[b_idx1] < DT[b_idx0])
            std::swap(DT[b_idx0], DT[b_idx1]);
        for(size_t idx = 2; idx < max_color; idx++) {
            node_p[idx] += opt_price_sum + sorted_g[idx].price;
            if(node_p[idx] < DT[b_idx1].price) {
                DT[b_idx1] = {idx, node_p[idx]};
                if(DT[b_idx1] < DT[b_idx0])
                    std::swap(DT[b_idx0], DT[b_idx1]);
            }

            node_p[idx] = 0;
        }
    }

    Price total_price = 0;
    std::vector<Gift> res_gifts(N);
    // Employee and what idx of gift which can't be used, idx as found in DT[emp][0/1].idx
    std::vector<std::pair<Employee, size_t>> q; q.reserve(N);

    for(const auto &director: directors) {
        const auto &dir_best = DT[2 * director];
        res_gifts[director] = sorted_g[dir_best.idx].idx;
        total_price += dir_best.price;
        for(const auto &sub: subordinates[director])
            q.emplace_back(sub, dir_best.idx);
    }

    const int_fast32_t q_max = N - directors.size();
    for(int_fast32_t curr_idx = 0; curr_idx < q_max; curr_idx++) {
        const auto &[emp, no_use] = q[curr_idx];
        auto best = DT[2 * emp];
        if(best.idx == no_use) best = DT[2 * emp + 1];
        res_gifts[emp] = sorted_g[best.idx].idx;
        for(const auto &sub: subordinates[emp])
            q.emplace_back(sub, best.idx);
    }

    return {total_price, res_gifts};
}

template<typename Allocator>
std::vector<size_t> rev_order(const std::vector<Employee> &directors,
                              const std::vector<std::vector<Employee, Allocator>> &subordinates) {
    auto N = static_cast<int_fast32_t>(subordinates.size());
    std::vector<Employee> res(N);
    auto ins = std::copy(directors.begin(), directors.end(), res.rbegin());

    for(int_fast32_t idx = N - 1; idx > 0; idx--)
        ins = std::copy(subordinates[res[idx]].begin(),
                        subordinates[res[idx]].end(),
                        ins);
    return res;
}

std::pair<Price, std::vector<Gift>> optimize_gifts(
        const std::vector<Employee> &boss,
        const std::vector<Price> &gift_price
) {
    if(gift_price.empty())
        return {0, {}};

    if(gift_price.size() == 1)
        return {gift_price[0] * boss.size(), std::vector<Gift>(1, 0)};

    std::vector<Employee> directors; directors.reserve(1000);
    std::vector<std::vector<Employee, small<Employee>>> subordinates(boss.size());

    size_t employee = 0;
    for(const auto &b: boss) {
        if(b != NO_EMPLOYEE)
            subordinates[b].emplace_back(employee);
        else
            directors.emplace_back(employee);
        employee++;
    }

    std::vector<Employee> order = rev_order(directors, subordinates);
    return more_gifts_case(boss, gift_price, subordinates, directors, order);
}

#ifndef __PROGTEST__

const std::tuple<Price, std::vector<Employee>, std::vector<Price>> EXAMPLES[] = {
        {44, {NO_EMPLOYEE,
                     NO_EMPLOYEE, 1,
                       NO_EMPLOYEE, 3, 3, 3, 6, 6, 6,
                     NO_EMPLOYEE, 10, 10, 10, 13, 13, 13, 16, 16, 16}, {4, 1}},
        {40, {NO_EMPLOYEE,
                     NO_EMPLOYEE, 1,
                       NO_EMPLOYEE, 3, 3, 3, 6, 6, 6,
                     NO_EMPLOYEE, 10, 10, 10, 13, 13, 13, 16, 16, 16}, {4, 5, 1}},
        {44, {NO_EMPLOYEE,
                     NO_EMPLOYEE, 1,
                       NO_EMPLOYEE, 3, 3, 3, 6, 6, 6,
                     NO_EMPLOYEE, 10, 10, 10, 13, 13, 13, 16, 16, 16}, {4, 8, 1}},
        {17, {1, 2, 3, 4, NO_EMPLOYEE},       {25, 4, 18, 3}},
        {16, {4, 4, 4, 4, NO_EMPLOYEE},       {25, 4, 18, 3}},
        {17, {4, 4, 3, 4, NO_EMPLOYEE},       {25, 4, 18, 3}},
        {24, {4, 4, 3, 4, NO_EMPLOYEE, 3, 3}, {25, 4, 18, 3}},
};

#define CHECK(cond, ...) do { \
    if (cond) break; \
    printf("Test failed: " __VA_ARGS__); \
    printf("\n"); \
    return false; \
  } while (0)

bool test(Price p, const std::vector<Employee> &boss, const std::vector<Price> &gp) {
    auto &&[sol_p, sol_g] = optimize_gifts(boss, gp);
    CHECK(sol_g.size() == boss.size(),
          "Size of the solution: expected %zu but got %zu.", boss.size(), sol_g.size());

    Price real_p = 0;
    for (Gift g: sol_g) real_p += gp[g];
    CHECK(real_p == sol_p, "Sum of gift prices is %llu but reported price is %llu.", real_p, sol_p);

    if (0) {
        for (Employee e = 0; e < boss.size(); e++) printf(" (%zu)%zu", e, sol_g[e]);
        printf("\n");
    }

    for (Employee e = 0; e < boss.size(); e++)
        CHECK(boss[e] == NO_EMPLOYEE || sol_g[boss[e]] != sol_g[e],
              "Employee %zu and their boss %zu has same gift %zu.", e, boss[e], sol_g[e]);

    CHECK(p == sol_p, "Wrong price: expected %llu got %llu.", p, sol_p);

    return true;
}

#undef CHECK

int main() {
    int ok = 0, fail = 0;
    for (auto &&[p, b, gp]: EXAMPLES) (test(p, b, gp) ? ok : fail)++;

    if (!fail) printf("Passed all %d tests!\n", ok);
    else printf("Failed %d of %d tests.", fail, fail + ok);
}

#endif
