#ifndef __PROGTEST__
#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <cstdint>
#include <deque>
#include <iomanip>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <random>
#include <set>
#include <stack>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

enum Point : size_t {};

struct Path {
  Point from, to;
  unsigned length;

  Path(size_t f, size_t t, unsigned l) : from{f}, to{t}, length{l} {}

  friend bool operator == (const Path& a, const Path& b) {
    return std::tie(a.from, a.to, a.length) == std::tie(b.from, b.to, b.length);
  }
  
  friend bool operator != (const Path& a, const Path& b) { return !(a == b); }
};

#endif



std::vector<Path> longest_track(size_t points, const std::vector<Path>& all_paths) {
    std::vector<char> in_count(points, 1);
    std::vector<std::vector<std::pair<ssize_t, ssize_t>>> G(points);

    for(const auto &path: all_paths) {
        in_count[path.to] = 0;
        G[path.from].emplace_back(path.to, path.length);
    }

    std::queue<size_t> q;
    std::vector<size_t> P(points);
    std::vector<ssize_t> D(points);

    for(size_t i = 0; i < points; ++i)
        if(in_count[i]) {
            q.push(i);
            P[i] = -1;
        }

    ssize_t max_D = -1;
    ssize_t fin = -1;

    while(!q.empty()) {
        // todo;
        auto curr = q.front(); q.pop();

        if(D[curr] > max_D) {
            max_D = D[curr];
            fin = static_cast<ssize_t>(curr);
        }

        for(const auto &[next, dist]: G[curr]) {
            if(D[next] >= D[curr] + dist) continue;
            q.push(next);
            D[next] = D[curr] + dist;
            P[next] = curr;
        }
    }

    std::vector<Path> res;

    while(true) {
        const auto from = P[fin];
        const auto to = fin;
        if(from == static_cast<size_t>(-1)) break;

        auto dist = D[to] - D[from];
        res.emplace_back(from, to, dist);
        fin = static_cast<ssize_t>(from);
    }

    std::reverse(res.begin(), res.end());
    return res;
}


#ifndef __PROGTEST__


struct Test {
  unsigned longest_track;
  size_t points;
  std::vector<Path> all_paths;
};

inline const Test TESTS[] = {
  {13, 5, { {3,2,10}, {3,0,9}, {0,2,3}, {2,4,1} } },
  {11, 5, { {3,2,10}, {3,1,4}, {1,2,3}, {2,4,1} } },
  {16, 8, { {3,2,10}, {3,1,1}, {1,2,3}, {1,4,15} } },
};

#define CHECK(cond, ...) do { \
    if (cond) break; \
    printf("Fail: " __VA_ARGS__); \
    printf("\n"); \
    return false; \
  } while (0)

bool run_test(const Test& t) {
  auto sol = longest_track(t.points, t.all_paths);

  unsigned length = 0;
  for (auto [ _, __, l ] : sol) length += l;

  CHECK(t.longest_track == length,
    "Wrong length: got %u but expected %u", length, t.longest_track);

  for (size_t i = 0; i < sol.size(); i++) {
    CHECK(std::count(t.all_paths.begin(), t.all_paths.end(), sol[i]),
      "Solution contains non-existent path: %zu -> %zu (%u)",
      sol[i].from, sol[i].to, sol[i].length);

    if (i > 0) CHECK(sol[i].from == sol[i-1].to,
      "Paths are not consecutive: %zu != %zu", sol[i-1].to, sol[i].from);
  }

  return true;
}
#undef CHECK

int main() {
  int ok = 0, fail = 0;

  for (auto&& t : TESTS) (run_test(t) ? ok : fail)++;
  
  if (!fail) printf("Passed all %i tests!\n", ok);
  else printf("Failed %u of %u tests.\n", fail, fail + ok);
}

#endif


