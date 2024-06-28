#ifndef __PROGTEST__

#include <cassert>
#include <cstdarg>
#include <iomanip>
#include <cstdint>
#include <iostream>
#include <memory>
#include <limits>
#include <optional>
#include <array>
#include <random>
#include <type_traits>

// We use std::set as a reference to check our implementation.
// It is not available in progtest :)
#include <set>

template<typename T>
struct Ref {
    [[nodiscard]] size_t size() const { return _data.size(); }

    [[nodiscard]] const T *find(const T &value) const {
        auto it = _data.find(value);
        if (it == _data.end()) return nullptr;
        return &*it;
    }

    bool insert(const T &value) { return _data.insert(value).second; }

    bool erase(const T &value) { return _data.erase(value); }

    [[nodiscard]] auto begin() const { return _data.begin(); }

    [[nodiscard]] auto end() const { return _data.end(); }

private:
    std::set<T> _data;
};

#endif


namespace config {
    // Enable to check that the tree is AVL balanced.
    inline constexpr bool CHECK_DEPTH = true;

    // Disable if your implementation does not have parent pointers
    inline constexpr bool PARENT_POINTERS = true;
}

/*
 *   5
 *  / \
 * 2   6
 */

// TODO implement
template<typename T>
struct Tree {
    using T_this = Tree<T>;
    ~Tree() noexcept {
        delete root;
    }

    struct Node {
        explicit Node(T &&t) noexcept: value{t} {};
        Node(T &&t, Node *parent) noexcept: value{t}, parent(parent) {};
        ~Node() noexcept {
            delete child[0];
            delete child[1];
        }

        bool operator<(const T &other) const {
            return value < other;
        }

        bool operator>(const T &other) const {
            return value > other;
        }

        bool operator==(const T &other) const {
            return !(value < other || other < value);
        }

        [[nodiscard]] const Node *from_index(size_t idx) const {
            auto c = this;
            while(true) {
                auto L_size = c->child[0] ? c->child[0]->size : 0;
                if(L_size == idx) return c;
                else if(L_size > idx) c = c->child[0];
                else {
                    c = c->child[1];
                    idx -= L_size + 1;
                }
            }
        }

        [[nodiscard]] Node *from_index(size_t idx) {
            return const_cast<Node *>(const_cast<const Node *>(this)->from_index(idx));
        }

        void update() {
            _depth = 0;
            size = 1;
            if(child[0]) {
                _depth = child[0]->_depth;
                size += child[0]->size;
            }
            if(child[1]) {
                _depth = std::max<uint16_t>(child[1]->_depth, _depth);
                size += child[1]->size;
            }
            _depth++;
        }

        [[nodiscard]] int16_t diff() const {
            return (child[1] ? child[1]->_depth : 0)
                 - (child[0] ? child[0]->_depth : 0);
        }

        void attach_tree(Node *other, uint8_t dir) {
            child[dir] = other;
            if(other) other->parent = this;
            update();
        }

        void get_parent_from(Node *other) {
            parent = other->parent;
            if(parent)
                parent->change_child(other, this);
        }

        void change_child(Node *old_child, Node *new_child) {
            child[child[1] == old_child] = new_child;
            if(new_child)
                new_child->parent = this;
            update();
        }

        void evict_child() {
            child.fill(nullptr);
        }

        T value;
        Node *parent = nullptr;
        std::array<Node *, 2> child{nullptr, nullptr};
        size_t size = 1;
    private:
        uint16_t _depth = 1;
    };

    [[nodiscard]] size_t size() const { return root ? root->size : 0; };

    [[nodiscard]] const T *find(const T &value) const {
        const Node *it = search(value);
        if(it && it->value == value)
            return &it->value;
        return nullptr;
    }

    bool insert(T value) {
        if(!root) {
            root = new Node{std::move(value)};
            return true;
        }

        Node *it = search(value);
        if(*it < value)
            it->child[1] = new Node{std::move(value), it};
        else if(*it > value)
            it->child[0] = new Node{std::move(value), it};
        else return false;

        root = fix(it);
        return true;
    }

    bool erase(const T &value) {
        Node *it = search(value);

        if(!it || *it != value)
            return false;

        Node *start;
        if(!it->child[0] && !it->child[1]) {
            start = it->parent;
            start->change_child(it, nullptr);
        }
        else if(!it->child[0] || !it->child[1]) {
            start = it->child[it->child[0] == nullptr];
            start->get_parent_from(it);
        }
        else if(!it->child[1]->child[0]) {
            start = it->child[1];
            start->get_parent_from(it);
            start->attach_tree(it->child[0], 0);
        }
        else {
            Node *next = it->child[1];
            while(next->child[0])
                next = next->child[0];

            start = next->parent;
            start->change_child(next, next->child[1]);

            next->get_parent_from(it);
            next->attach_tree(it->child[0], 0);
            next->attach_tree(it->child[1], 1);
        }

        it->evict_child();
        delete it;
        root = fix(start);
        return true;
    }

    // Needed to test the structure of the tree.
    // Replace Node with the real type of your nodes
    // and implementations with the ones matching
    // your attributes.
    struct TesterInterface {
        // using Node = ...
        static const Node *root(const Tree *t) { return t->root; }

        // Parent of root must be nullptr, ignore if config::PARENT_POINTERS == false
        static const Node *parent(const Node *n) { return n->parent; }

        static const Node *right(const Node *n) { return n->child[1]; }

        static const Node *left(const Node *n) { return n->child[0]; }

        static const T &value(const Node *n) { return n->value; }
    };

private:
        [[nodiscard]] Node *fix(Node *current) {
            Node *prev;
            for(prev = current;
                current;
                current = current->parent)
            {
                current->update();

                auto diff_curr = current->diff();
                if(-1 <= diff_curr && diff_curr <= 1) {
                    prev = current;
                    continue;
                }
                else if(diff_curr < -1)
                    prev = current->child[0];
                else
                    prev = current->child[1];

                auto diff_prev = prev->diff();

                /* diff_curr vs. diff_prev
                 *      -2      2
                 *  -1   R      RL
                 *   0   R      L
                 *   1   LR     L
                 */

                Node* (Tree<T>::*rotation)(Node*, Node *);
                if(diff_curr < 0) {
                    if(diff_prev > 0)
                        rotation = &T_this::LR_rot;
                    else
                        rotation = &T_this::R_rot;
                }
                else {
                    if(diff_prev < 0)
                        rotation = &T_this::RL_rot;
                    else
                        rotation = &T_this::L_rot;
                }

                current = prev = (this->*rotation)(current, prev);
            }

            return prev;
        }

        [[nodiscard]] Node * RL_rot(Node *x, Node *y) {
            return L_rot(x, R_rot(y, y->child[0]));
        }

        [[nodiscard]] Node * LR_rot(Node *x, Node *y) {
            return R_rot(x ,L_rot(y, y->child[1]));
        }

        [[nodiscard]] Node * L_rot(Node *x, Node *y) {
            y->get_parent_from(x);
            x->attach_tree(y->child[0], 1);
            y->attach_tree(x, 0);
            return y;
        }

        [[nodiscard]] Node * R_rot(Node *x, Node *y) {
            y->get_parent_from(x);
            x->attach_tree(y->child[1], 0);
            y->attach_tree(x, 1);
            return y;
        }

    [[nodiscard]] const Node *search(const T &value) const {
        Node *it = root;

        while(it) {
            if(*it < value && it->child[1]) {
                it = it->child[1];
            }
            else if(*it > value && it->child[0]) {
                it = it->child[0];
            }
            else break;
        }
        return it;
    }

    [[nodiscard]] Node *search(const T &value) {
        return const_cast<Node *>(const_cast<const T_this *>(this)->search(value));
    }

    Node *root = nullptr;
};


#ifndef __PROGTEST__

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

template<typename T>
struct Tester {
    Tester() = default;

    void size() const {
        size_t r = ref.size();
        size_t t = tested.size();
        if (r != t)
            throw TestFailed(fmt("Size: got %zu but expected %zu.", t, r));
    }

    void find(const T &x) const {
        auto r = ref.find(x);
        auto t = tested.find(x);
        bool found_r = r != nullptr;
        bool found_t = t != nullptr;

        if (found_r != found_t) _throw("Find mismatch", found_r);
        if (found_r && *t != x) throw TestFailed("Find: found different value");
    }

    void insert(const T &x, bool check_tree_ = false) {
        auto succ_r = ref.insert(x);
        auto succ_t = tested.insert(x);
        if (succ_r != succ_t) _throw("Insert mismatch", succ_r);
        size();
        if (check_tree_) check_tree();
    }

    void erase(const T &x, bool check_tree_ = false) {
        bool succ_r = ref.erase(x);
        auto succ_t = tested.erase(x);
        if (succ_r != succ_t) _throw("Erase mismatch", succ_r);
        size();
        if (check_tree_) check_tree();
    }

    struct NodeCheckResult {
        const T *min = nullptr;
        const T *max = nullptr;
        int depth = -1;
        size_t size = 0;
    };

    void check_tree() const {
        using TI = typename Tree<T>::TesterInterface;
        auto ref_it = ref.begin();
        bool check_value_failed = false;
        auto check_value = [&](const T &v) {
            if (check_value_failed) return;
            check_value_failed = (ref_it == ref.end() || *ref_it != v);
            if (!check_value_failed) ++ref_it;
        };

        auto r = check_node(TI::root(&tested), decltype(TI::root(&tested))(nullptr), check_value);
        size_t t_size = tested.size();

        if (t_size != r.size)
            throw TestFailed(
                    fmt("Check tree: size() reports %zu but expected %zu.", t_size, r.size));

        if (check_value_failed)
            throw TestFailed(
                    "Check tree: element mismatch");

        size();
    }

    template<typename Node, typename F>
    NodeCheckResult check_node(const Node *n, const Node *p, F &check_value) const {
        if (!n) return {};

        using TI = typename Tree<T>::TesterInterface;
        if constexpr (config::PARENT_POINTERS) {
            if (TI::parent(n) != p)
                throw TestFailed("Parent mismatch.");
        }

        auto l = check_node(TI::left(n), n, check_value);
        check_value(TI::value(n));
        auto r = check_node(TI::right(n), n, check_value);

        if (l.max && !(*l.max < TI::value(n)))
            throw TestFailed("Max of left subtree is too big.");
        if (r.min && !(TI::value(n) < *r.min))
            throw TestFailed("Min of right subtree is too small.");

        if (config::CHECK_DEPTH && abs(l.depth - r.depth) > 1)
            throw TestFailed(fmt(
                    "Tree is not avl balanced: left _depth %i and right _depth %i.",
                    l.depth, r.depth
            ));

        return {
                l.min ? l.min : &TI::value(n),
                r.max ? r.max : &TI::value(n),
                std::max(l.depth, r.depth) + 1, 1 + l.size + r.size
        };
    }

    static void _throw(const char *msg, bool s) {
        throw TestFailed(fmt("%s: ref %s.", msg, s ? "succeeded" : "failed"));
    }

    Tree<T> tested;
    Ref<T> ref;
};


void test_insert() {
    Tester<int> t;

    for (int i = 0; i < 10; i++) t.insert(i, true);
    for (int i = -10; i < 20; i++) t.find(i);

    for (int i = 0; i < 10; i++) t.insert((1 + i * 7) % 17, true);
    for (int i = -10; i < 20; i++) t.find(i);
}

void test_erase() {
    Tester<int> t;

    for (int i = 0; i < 10; i++) t.insert((1 + i * 7) % 17, true);
    for (int i = -10; i < 20; i++) t.find(i);

    for (int i = 3; i < 22; i += 2)
        t.erase(i, true);
    for (int i = -10; i < 20; i++) t.find(i);

    for (int i = 0; i < 10; i++)
        t.insert((1 + i * 13) % 17 - 8, true);
    for (int i = -10; i < 20; i++) t.find(i);

    for (int i = -4; i < 10; i++) t.erase(i, true);
    for (int i = -10; i < 20; i++) t.find(i);
}

enum RandomTestFlags : unsigned {
    SEQ = 1, NO_ERASE = 2, CHECK_TREE = 4
};

void test_random(size_t size, unsigned flags = 0) {
    Tester<size_t> t;
    std::mt19937 my_rand(24707 + size);

    bool seq = flags & SEQ;
    bool erase = !(flags & NO_ERASE);
    bool check_tree = flags & CHECK_TREE;

    for (size_t i = 0; i < size; i++)
        t.insert(seq ? 2 * i : my_rand() % (3 * size), check_tree);

    t.check_tree();

    for (size_t i = 0; i < 3 * size + 1; i++) t.find(i);

    for (size_t i = 0; i < 30 * size; i++)
        switch (my_rand() % 5) {
            case 1:
                t.insert(my_rand() % (3 * size), check_tree);
                break;
            case 2:
                if (erase) t.erase(my_rand() % (3 * size), check_tree);
                break;
            default:
                t.find(my_rand() % (3 * size));
        }

    t.check_tree();
}

int main() {
    std::vector<int> hs(20);

    hs[0] = 0;
    hs[1] = 1;

    for(int i = 2; i < 20; i++) {
        hs[i] = hs[i-1] + hs[i-2] + 1;
        std::cout << i << ": " << hs[i] << std::endl;
    }


    try {
        std::cout << "Insert test..." << std::endl;
        test_insert();

        std::cout << "Erase test..." << std::endl;
        test_erase();

        std::cout << "Tiny random test..." << std::endl;
        test_random(20, CHECK_TREE);

        std::cout << "Small random test..." << std::endl;
        test_random(200, CHECK_TREE);

        std::cout << "Big random test..." << std::endl;
        test_random(50'000);

        std::cout << "Big sequential test..." << std::endl;
        test_random(50'000, SEQ);

        std::cout << "All tests passed." << std::endl;
    } catch (const TestFailed &e) {
        std::cout << "Test failed: " << e.what() << std::endl;
    }
}

#endif
