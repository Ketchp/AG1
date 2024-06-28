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

// We use std::vector as a reference to check our implementation.
// It is not available in progtest :)
#include <vector>

template<typename T>
struct Ref {
    [[nodiscard]] bool empty() const { return _data.empty(); }

    [[nodiscard]] size_t size() const { return _data.size(); }

    const T &operator[](size_t index) const { return _data.at(index); }

    T &operator[](size_t index) { return _data.at(index); }

    void insert(size_t index, T value) {
        if (index > _data.size()) throw std::out_of_range("oops");
        _data.insert(_data.begin() + index, std::move(value));
    }

    T erase(size_t index) {
        T ret = std::move(_data.at(index));
        _data.erase(_data.begin() + index);
        return ret;
    }

    [[nodiscard]] auto begin() const { return _data.begin(); }

    [[nodiscard]] auto end() const { return _data.end(); }

private:
    std::vector<T> _data;
};

#endif


namespace config {
    inline constexpr bool PARENT_POINTERS = true;
    inline constexpr bool CHECK_DEPTH = true;
}


template<typename T_val>
struct Array {
    using T_this = Array<T_val>;

    ~Array() noexcept {
        delete root;
    }

    struct Node {
        explicit Node(T_val &&t) noexcept
            : value{t} {};

        Node(T_val &&t, Node *parent) noexcept
            : value{t}, parent{parent} {};

        ~Node() noexcept {
            delete child[0];
            delete child[1];
        }

        bool operator<(const T_val &other) const {
            return value < other;
        }

        bool operator>(const T_val &other) const {
            return value > other;
        }

        bool operator==(const T_val &other) const {
            return !(value < other || other < value);
        }

        [[nodiscard]] const Node *from_index(size_t idx) const {
            if(size <= idx)
                throw std::out_of_range("Out of range.");
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
            if (child[0]) {
                _depth = child[0]->_depth;
                size += child[0]->size;
            }
            if (child[1]) {
                _depth = std::max<uint16_t>(child[1]->_depth, _depth);
                size += child[1]->size;
            }
            ++_depth;
        }

        [[nodiscard]] int16_t diff() const {
            return (child[1] ? child[1]->_depth : 0)
                   - (child[0] ? child[0]->_depth : 0);
        }

        void attach_tree(T_val val, uint8_t dir) {
            child[dir] = new Node{std::move(val), this};
        }

        void attach_tree(Node *other, uint8_t dir) {
            child[dir] = other;
            if (other) other->parent = this;
            update();
        }

        void get_parent_from(Node *other) {
            parent = other->parent;
            if (parent)
                parent->change_child(other, this);
        }

        void change_child(Node *old_child, Node *new_child) {
            child[child[1] == old_child] = new_child;
            if (new_child)
                new_child->parent = this;
            update();
        }

        void evict_child() {
            child.fill(nullptr);
        }

        T_val value;
        Node *parent = nullptr;
        std::array<Node *, 2> child{nullptr, nullptr};
        size_t size = 1;
    private:
        uint16_t _depth = 1;
    };

    [[nodiscard]] bool empty() const { return !root; };

    [[nodiscard]] size_t size() const  { return root ? root->size : 0; };

    [[nodiscard]] const T_val &operator[](size_t index) const {
        return root->from_index(index)->value;
    }

    T_val &operator[](size_t index) {
        return root->from_index(index)->value;
    }

    void insert(size_t index, T_val value) {
        if(!root) {
            root = new Node{std::move(value)};
            return;
        }

        Node *it;
        if(index == 0) {
            it = begin();
            it->attach_tree(std::move(value), 0);
        }
        else {
            it = root->from_index(--index);
            if(!it->child[1])
                it->attach_tree(std::move(value), 1);
            else {
                it = it->child[1];
                while (it->child[0])
                    it = it->child[0];
                it->attach_tree(std::move(value), 0);
            }
        }

        root = fix(it);
    }

    T_val erase(size_t index) {
        if(!root)
            throw std::out_of_range("Empty array.");

        Node *it = root->from_index(index);

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
        T_val ret = std::move(it->value);
        delete it;
        root = fix(start);
        return ret;
    }

    // Needed to test the structure of the tree.
    // Replace Node with the real type of your nodes
    // and implementations with the ones matching
    // your attributes.
    struct TesterInterface {
        static const Node *root(const Array *t) { return t->root; }

        // Parent of root must be nullptr, ignore if config::PARENT_POINTERS == false
        static const Node *parent(const Node *n) { return n->parent; }

        static const Node *right(const Node *n) { return n->child[1]; }

        static const Node *left(const Node *n) { return n->child[0]; }

        static const T_val &value(const Node *n) { return n->value; }
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

            Node* (T_this::*rotation)(Node*, Node *);
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

//    [[nodiscard]] const Node *search(const T_val &value) const {
//        Node *it = root;
//
//        while(it) {
//            if(*it < value && it->child[1]) {
//                it = it->child[1];
//            }
//            else if(*it > value && it->child[0]) {
//                it = it->child[0];
//            }
//            else break;
//        }
//        return it;
//    }
//
//    [[nodiscard]] Node *search(const T_val &value) {
//        return const_cast<Node *>(const_cast<const T_this *>(this)->search(value));
//    }

    Node *begin() {
        if(!root) return nullptr;
        auto c = root;
        while(c->child[0]) c = c->child[0];
        return c;
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

    size_t size() const {
        bool te = tested.empty();
        size_t r = ref.size();
        size_t t = tested.size();
        if (te != !t)
            throw TestFailed(fmt("Size: size %zu but empty is %s.",
                                 t, te ? "true" : "false"));
        if (r != t) throw TestFailed(fmt("Size: got %zu but expected %zu.", t, r));
        return r;
    }

    const T &operator[](size_t index) const {
        const T &r = ref[index];
        const T &t = tested[index];
        if (r != t) throw TestFailed("Op [] const mismatch.");
        return t;
    }

    void assign(size_t index, T x) {
        ref[index] = x;
        tested[index] = std::move(x);
        operator[](index);
    }

    void insert(size_t i, T x, bool check_tree_ = false) {
        ref.insert(i, x);
        tested.insert(i, std::move(x));
        size();
        if (check_tree_) check_tree();
    }

    T erase(size_t i, bool check_tree_ = false) {
        T r = ref.erase(i);
        T t = tested.erase(i);
        if (r != t) TestFailed(fmt("Erase mismatch at %zu.", i));
        size();
        if (check_tree_) check_tree();
        return t;
    }

    void check_tree() const {
        using TI = typename Array<T>::TesterInterface;
        auto ref_it = ref.begin();
        bool check_value_failed = false;
        auto check_value = [&](const T &v) {
            if (check_value_failed) return;
            check_value_failed = (ref_it == ref.end() || *ref_it != v);
            if (!check_value_failed) ++ref_it;
        };

        size();

        check_node(TI::root(&tested), decltype(TI::root(&tested))(nullptr), check_value);

        if (check_value_failed)
            throw TestFailed(
                    "Check tree: element mismatch");
    }

    template<typename Node, typename F>
    int check_node(const Node *n, const Node *p, F &check_value) const {
        if (!n) return -1;

        using TI = typename Array<T>::TesterInterface;
        if constexpr (config::PARENT_POINTERS) {
            if (TI::parent(n) != p) throw TestFailed("Parent mismatch.");
        }

        auto l_depth = check_node(TI::left(n), n, check_value);
        check_value(TI::value(n));
        auto r_depth = check_node(TI::right(n), n, check_value);

        if (config::CHECK_DEPTH && abs(l_depth - r_depth) > 1)
            throw TestFailed(fmt(
                    "Tree is not avl balanced: left depth %i and right depth %i.",
                    l_depth, r_depth
            ));

        return std::max(l_depth, r_depth) + 1;
    }

    static void _throw(const char *msg, bool s) {
        throw TestFailed(fmt("%s: ref %s.", msg, s ? "succeeded" : "failed"));
    }

    Array<T> tested;
    Ref<T> ref;
};


void test_insert() {
    Tester<int> t;

    for (int i = 0; i < 10; i++) t.insert(i, i, true);
    for (int i = 0; i < 10; i++) t.insert(i, -i, true);
    for (size_t i = 0; i < t.size(); i++) t[i];

    for (int i = 0; i < 5; i++) t.insert(15, (1 + i * 7) % 17, true);
    for (int i = 0; i < 10; i++) t.assign(2 * i, 3 * t[2 * i]);
    for (size_t i = 0; i < t.size(); i++) t[i];
}

void test_erase() {
    Tester<int> t;

    for (int i = 0; i < 10; i++) t.insert(i, i, true);
    for (int i = 0; i < 10; i++) t.insert(i, -i, true);

    for (size_t i = 3; i < t.size(); i += 2) t.erase(i, true);
    for (size_t i = 0; i < t.size(); i++) t[i];

    for (int i = 0; i < 5; i++) t.insert(3, (1 + i * 7) % 17, true);
    for (size_t i = 1; i < t.size(); i += 3) t.erase(i, true);

    for (int i = 0; i < 20; i++) t.insert(3, 100 + i, true);

    for (int i = 0; i < 5; i++) t.erase(t.size() - 1, true);
    for (int i = 0; i < 5; i++) t.erase(0, true);

    for (int i = 0; i < 4; i++) t.insert(i, i, true);
    for (size_t i = 0; i < t.size(); i++) t[i];
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

    for (size_t i = 0; i < size; i++) {
        size_t pos = seq ? 0 : my_rand() % (i + 1);
        t.insert(pos, my_rand() % (3 * size), check_tree);
    }

    t.check_tree();

    for (size_t i = 0; i < t.size(); i++) t[i];

    for (size_t i = 0; i < 30 * size; i++)
        switch (my_rand() % 7) {
            case 1: {
                if (!erase && i % 3 == 0) break;
                size_t pos = seq ? 0 : my_rand() % (t.size() + 1);
                t.insert(pos, my_rand() % 1'000'000, check_tree);
                break;
            }
            case 2:
                if (erase) t.erase(my_rand() % t.size(), check_tree);
                break;
            case 3:
                t.assign(my_rand() % t.size(), 155 + i);
                break;
            default:
                t[my_rand() % t.size()];
        }

    t.check_tree();
}

int main() {
    try {
        std::cout << "Insert test..." << std::endl;
        test_insert();

        std::cout << "Erase test..." << std::endl;
        test_erase();

        std::cout << "Tiny random test..." << std::endl;
        test_random(20, CHECK_TREE);

        std::cout << "Small random test..." << std::endl;
        test_random(200, CHECK_TREE);

        std::cout << "Bigger random test..." << std::endl;
        test_random(5'000);

        std::cout << "Bigger sequential test..." << std::endl;
        test_random(5'000, SEQ);

        std::cout << "All tests passed." << std::endl;
    } catch (const TestFailed &e) {
        std::cout << "Test failed: " << e.what() << std::endl;
    }
}

#endif


