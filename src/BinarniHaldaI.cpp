#ifndef __PROGTEST__

#include <cstdarg>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <array>

// Note that std::pop_heap and std::push_heap are disabled
#include <algorithm>

#include <vector>
#include <deque>


struct TestFailed : std::runtime_error {
    using std::runtime_error::runtime_error;
};

std::string fmt(const char *f, ...) {
    va_list args1;
    va_list args2;
    va_start(args1, f);
    va_copy(args2, args1);

    std::string buf(vsnprintf(nullptr, 0, f, args1), '\0');
    va_end(args1);

    vsnprintf(buf.data(), buf.size() + 1, f, args2);
    va_end(args2);

    return buf;
}

// We use std::multiset as a reference to check our implementation.
// It is not available in progtest :)
#include <set>

// On the other hand, Link (as seen in the harder version of this task)
// is still available in progtest.

#endif


// TODO implement
template<typename T, typename Comp = std::less<T> >
struct BinaryHeap {
    BinaryHeap() noexcept = default;

    explicit BinaryHeap(Comp comp) noexcept : comp(comp) {};

    [[nodiscard]] bool empty() const {return inner.empty();};

    [[nodiscard]] size_t size() const {return inner.size();};

    [[nodiscard]] const T &min() const {return inner.at(0);};

    T extract_min() {
        T min_val = std::move(inner.at(0));
        inner.front() = std::move(inner.back());
        inner.pop_back();
        bubble_down();
        return min_val;
    };

    void push(T val) {
        inner.push_back(std::move(val));
        bubble_up();
    };

    // Helpers to enable testing.
    struct TestHelper {
        static const T &index_to_value(const BinaryHeap &H, size_t index) {
            return H.inner[index];
        }

        static size_t root_index() { return 0; }

        static size_t parent(size_t index) {
            return (index - 1) / 2;
        }

        static size_t child(size_t parent, size_t ith) {
            return 2 * parent + ith + 1;
        }
    };

protected:
    void bubble_down(size_t start = 1) {
        const size_t sz1 = index_end();
        size_t son0, son1;
        while((son1 = (son0 = start * 2) + 1) < sz1) {
            size_t smaller_son = comp_idx(son0, son1) ? son0 : son1;

            if(!comp_idx(smaller_son, start))
                break;
            std::swap(inner[smaller_son - 1],
                      inner[start - 1]);
            start = smaller_son;
        }

        if(son0 < sz1) {
            if(comp_idx(son0, start))
                std::swap(inner[son0 - 1],
                          inner[start - 1]);
        }
    };

    void bubble_up(size_t start = 0) {
        size_t sz1 = index_end();
        if(start == 0) start = sz1 - 1;

        while(start > 1) {
            size_t parent = start / 2;
            if(!comp_idx(start, parent))
                break;
            std::swap(inner[start - 1],
                      inner[parent - 1]);
            start = parent;
        }
    };

    [[nodiscard]] inline bool comp_idx(size_t idx0, size_t idx1) const {
        return comp(inner[idx0 - 1], inner[idx1 - 1]);
    }

    [[nodiscard]] inline size_t index_end() const {
        return size() + 1;
    }

    std::vector<T> inner;
    Comp comp{};
};


#ifndef __PROGTEST__

#define CHECK(cond, ...) do { \
    if (!(cond)) throw Error(fmt(__VA_ARGS__)); \
  } while (0)

template<
        typename T, typename Cmp,
        template<typename, typename> typename Tested_
>
class HeapTester {
    struct Node;
    using TElem = const Node *;

    struct CmpInternal {
        const HeapTester *t;

        bool operator()(const Node &a, const Node &b) const { return t->_cmp(a.value, b.value); }

        bool operator()(const TElem &a, const TElem &b) const {
            return t->_cmp(a->value, b->value);
        }
    };

    using It = typename std::multiset<Node, CmpInternal>::const_iterator;

    struct Node {
        T value;
        mutable It rref;

        explicit Node(T value) : value(std::move(value)) {}

        friend class HeapTester;
    };

public:
    struct Error : std::runtime_error {
        using std::runtime_error::runtime_error;
    };


    explicit HeapTester(Cmp cmp = {})
            : _cmp(std::move(cmp)), _ref(CmpInternal{this}), _t(CmpInternal{this}) {}


    [[nodiscard]] bool empty() const {
        CHECK(_t.empty() == _ref.empty(),
              "_t.empty() == %d, _ref.empty() == %d", _t.empty(), _ref.empty());
        return _t.empty();
    }

    [[nodiscard]] size_t size() const {
        CHECK(_t.size() == _ref.size(),
              "_t.size() == %zu, _ref.size() == %zu", _t.size(), _ref.size());
        return _t.size();
    }


    void push(T val) {
        auto it = _ref.insert(Node(std::move(val)));
        it->rref = it;
        _t.push(&*it);
    }

    [[nodiscard]] const T &min() const {
        const Node *n = _min();
        _check_node(n);
        return n->value;
    }

    T extract_min() {
        const Node *r = _min();
        _check_node(r);
        const Node *t = _t.extract_min();
        CHECK(r == t, "_t.min() != _t.extract_min()");
        return std::move(_ref.extract(r->rref).value().value);
    }

    void check_structure() {
        using Helper = typename decltype(_t)::TestHelper;

        auto node_at_index = [&](size_t i) {
            return Helper::index_to_value(_t, i);
        };

        std::vector<const Node *> seen;
        const size_t root = Helper::root_index();
        const size_t max = root + size();
        for (size_t i = root; i < max; i++) {
            const Node *n = node_at_index(i);
            _check_node(n);
            seen.push_back(n);

            if (i != root)
                CHECK(!_cmp(n->value, node_at_index(Helper::parent(i))->value), "parent > son");

            CHECK(Helper::parent(Helper::child(i, 0)) == i, "n->left->parent != n");
            CHECK(Helper::parent(Helper::child(i, 1)) == i, "n->right->parent != n");
        }

        std::sort(seen.begin(), seen.end());
        CHECK(std::unique(seen.begin(), seen.end()) == seen.end(),
              "Some node was seen multiple times");
    }

private:
    void _check_node(const Node *n) const {
        CHECK(&*n->rref == n, "Node mismatch (rref)");
    }

    [[nodiscard]] const Node *_min() const {
        const Node &rmin = *_ref.begin();
        const Node *tmin = _t.min();
        CHECK(!_cmp(rmin.value, tmin->value), "_ref.min() < _t.min()");
        CHECK(!_cmp(tmin->value, rmin.value), "_t.min() < _ref.min()");
        return tmin;
    }

    Cmp _cmp;
    std::multiset<Node, CmpInternal> _ref;
    Tested_<TElem, CmpInternal> _t;
};

#undef CHECK

template<typename Heap>
void run_test(int max, bool check_structure = false) {
    Heap H;

    auto check = [&]() {
        if (check_structure) H.check_structure();
    };

    for (int i = 0; i < max; i++) {
        H.push((i * 991) % (5 * max));
        check();
    }

    for (int i = 0; i < 2 * max; i++) {
        if(H.empty())
            throw(TestFailed("Not empty."));
        H.extract_min();
        check();
        H.push((i * 991) % (7 * max));
        check();
    }
}

template<typename T, typename Cmp = std::less<T> >
using Tester = HeapTester<T, Cmp, BinaryHeap>;

int main() {
    {
        BinaryHeap<int> H;

        try {
            H.min();
            throw TestFailed("min on empty heap did not throw");
        } catch (const std::out_of_range &) {}

        try {
            H.extract_min();
            throw TestFailed("extract_min on empty heap did not throw");
        } catch (const std::out_of_range &) {}
    }

    try {
        std::cout << "Small test..." << std::endl;
        run_test<Tester<int>>(20);
        run_test<Tester<int>>(20, true);
        run_test<Tester<int>>(1'000, true);


        std::cout << "Big test..." << std::endl;
        run_test<Tester<int>>(500'000);

        std::cout << "All tests passed." << std::endl;
    } catch (const TestFailed &) {}
}

#endif


