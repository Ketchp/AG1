#ifndef __PROGTEST__

//#include <cassert>
//#include <iomanip>
#include <cstdint>
#include <iostream>
//#include <memory>
#include <limits>
#include <optional>
#include <algorithm>
#include <bitset>
//#include <list>
#include <array>
#include <vector>
//#include <deque>
//#include <unordered_set>
//#include <unordered_map>
//#include <stack>
//#include <queue>
//#include <random>
//#include <type_traits>

#endif

template<typename T_Node, typename T_val>
struct Node {
    using T_this = Node<T_Node, T_val>;
    using T_value = T_val;

    explicit Node(T_val t) noexcept
        : value{std::move(t)}
    {};

    Node(T_val t, T_Node *parent) noexcept
        : value{std::move(t)}, parent{parent}
    {};

    template <std::random_access_iterator T_it>
    static T_Node *from_iter(T_it begin, T_it end) {
        if(begin == end)
            return nullptr;

        auto mid_it = begin + (end - begin) / 2;
        auto node = new T_Node{*mid_it};

        node->attach_tree(from_iter(begin, mid_it), 0);
        node->attach_tree(from_iter(mid_it + 1, end), 1);

        return node;
    }

    ~Node() noexcept {
        delete child[0];
        delete child[1];
    }

    void update() {
        depth = 1 + std::max(depth_L(), depth_R());
    }

    [[nodiscard]] ssize_t diff() const {
        return depth_R() - depth_L();
    }

    void attach_tree(T_Node *other, uint8_t dir) {
        child[dir] = other;
        if (other)
            other->parent = static_cast<T_Node *>(this);
        static_cast<T_Node *>(this)->update();
    }

    void get_parent_from(T_Node *other) {
        parent = other->parent;
        if (parent)
            parent->change_child(other, static_cast<T_Node *>(this));
    }

    void change_child(T_Node *old_child, T_Node *new_child) {
        child[child[1] == old_child] = new_child;
        if (new_child)
            new_child->parent = static_cast<T_Node *>(this);
        static_cast<T_Node *>(this)->update();
    }

    void evict_child() {
        child.fill(nullptr);
    }

    [[nodiscard]] const T_Node *next() const {
        if(right()) {
            auto c = right();
            while(c->left()) c = c->left();
            return c;
        }
        auto c = static_cast<const T_Node *>(this);
        while(c)
            if(!c->parent || c->parent->right() == c)
                c = c->parent;
            else return c->parent;
        return c;
    }

    [[nodiscard]] T_Node *next() {
        return const_cast<T_Node *>(const_cast<const T_this *>(this)->next());
    }


    [[nodiscard]] T_Node *left() { return child[0]; }
    [[nodiscard]] T_Node *right() { return child[1]; }
    [[nodiscard]] const T_Node *left() const { return child[0]; }
    [[nodiscard]] const T_Node *right() const { return child[1]; }

    T_val value;
    T_Node *parent = nullptr;
    std::array<T_Node *, 2> child{nullptr, nullptr};
private:
    [[nodiscard]] ssize_t depth_L() const { return child[0] ? child[0]->depth : 0; };
    [[nodiscard]] ssize_t depth_R() const { return child[1] ? child[1]->depth : 0; };

    size_t depth = 1;
};


template<template<typename> typename T_Node_temp, typename T_val>
struct IndexableNodeImpl: public Node<T_Node_temp<T_val>, T_val> {
    using Node<T_Node_temp<T_val>, T_val>::Node;
    using T_Node = T_Node_temp<T_val>;
    using T_base = Node<T_Node, T_val>;
    using T_this = IndexableNodeImpl<T_Node_temp, T_val>;

    void update() {
        T_base::update();
        _size = 1 + size_L() + size_R();
    }

    [[nodiscard]] const T_Node *from_index(size_t idx) const {
        if(_size <= idx)
            throw std::out_of_range("Out of range.");
        auto *c = static_cast<const T_Node *>(this);
        while(true) {
            auto L_size = c->child[0] ? c->child[0]->_size : 0;
            if(L_size == idx) return c;
            else if(L_size > idx)
                c = static_cast<const T_Node *>(c->child[0]);
            else {
                idx -= L_size + 1;
                c = static_cast<const T_Node *>(c->child[1]);
            }
        }
    }

    [[nodiscard]] T_Node *from_index(size_t idx) {
        return const_cast<T_Node *>(const_cast<const T_this *>(this)->from_index(idx));
    }

    [[nodiscard]] size_t index() const {
        size_t idx = 0;
        auto c = static_cast<const T_Node *>(this);

        if(c->left()) idx += c->left()->size();

        while(c->parent) {
            if(c->parent->right() == c) {
                idx++;
                if(c->parent->left())
                    idx += c->parent->left()->size();
            }
            c = c->parent;
        }
        return idx;
    }

    [[nodiscard]] size_t size() const { return _size; };

private:
    [[nodiscard]] size_t size_L() const {
        return this->child[0] ? static_cast<T_this *>(this->child[0])->_size : 0;
    }

    [[nodiscard]] size_t size_R() const {
        return this->child[1] ? static_cast<T_this *>(this->child[1])->_size : 0;
    }

    size_t _size = 1;
};


template<template<typename> typename T_Node_temp, std::integral T_val>
struct LinesNodeImpl: public IndexableNodeImpl<T_Node_temp, T_val> {
    using T_this = LinesNodeImpl<T_Node_temp, T_val>;
    using T_base = IndexableNodeImpl<T_Node_temp, T_val>;
    using T_Node = T_base::T_Node;

    explicit LinesNodeImpl(T_val value, auto ...rest)
        : IndexableNodeImpl<T_Node_temp, T_val>{value, rest...}, sum(value)
    {}

    void update() {
        T_base::update();
        sum = static_cast<T_Node *>(this)->value + sum_L() + sum_R();
    }

    [[nodiscard]] size_t start() const {
        size_t res = 0;
        auto c = static_cast<const T_Node *>(this);

        if(c->left()) res += c->left()->sum;

        while(c->parent) {
            if(c->parent->right() == c) {
                res += c->parent->value;
                if(c->parent->left())
                    res += c->parent->left()->sum;
            }
            c = c->parent;
        }
        return res;
    }

    [[nodiscard]] const T_Node *from_char_index(size_t idx) const {
        auto *c = static_cast<const T_Node *>(this);
        if(sum == idx) {
            while(c->right()) c = c->right();
            if(c->value == 0) return c;
        }

        if(sum <= idx)
            throw std::out_of_range("Out of range.");

        while(true) {
            auto L_size = c->child[0] ? c->child[0]->sum : 0;
            if(L_size <= idx && idx < L_size + c->value) return c;
            else if(idx < L_size)
                c = static_cast<const T_Node *>(c->child[0]);
            else {
                idx -= L_size + c->value;
                c = static_cast<const T_Node *>(c->child[1]);
            }
        }
    }

    T_Node *from_char_index(size_t idx) {
        return const_cast<T_Node *>(const_cast<const T_this *>(this)->from_char_index(idx));
    }

    void recursive_update() {
        auto c = static_cast<T_Node *>(this);
        while(c) {
            c->update();
            c = c->parent;
        }
    }

    void update_value(T_val new_value) {
        this->value = new_value;
        recursive_update();
    }

    void add_char() {
        add_chars(1);
    }

    void add_chars(T_val count) {
        this->value += count;
        recursive_update();
    }

    void pop_char() {
        this->value--;
        recursive_update();
    }

private:
    [[nodiscard]] size_t sum_L() const {
        return this->child[0] ? static_cast<T_this *>(this->child[0])->sum : 0;
    }

    [[nodiscard]] size_t sum_R() const {
        return this->child[1] ? static_cast<T_this *>(this->child[1])->sum : 0;
    }

    T_val sum = 0;
};

template<typename T_val>
struct IndexableNode: public IndexableNodeImpl<IndexableNode, T_val> {
    using IndexableNodeImpl<IndexableNode, T_val>::IndexableNodeImpl;
};

template<typename T_val>
struct LinesNode: public LinesNodeImpl<LinesNode, T_val> {
    using LinesNodeImpl<LinesNode, size_t>::LinesNodeImpl;
};


template<typename T>
concept IndexableConcept = requires (T t, const T tc, size_t idx) {
    typename T::T_Node;
    { t.from_index(idx) } -> std::same_as<T *>;
    { tc.from_index(idx) } -> std::same_as<const T *>;
};

template<typename T>
concept LineConcept = requires (T t, const T tc, size_t idx) {
    typename T::T_Node;
    { t.from_char_index(idx) } -> std::same_as<T *>;
    { tc.from_char_index(idx) } -> std::same_as<const T *>;
    { t.add_char() } -> std::same_as<void>;
    { t.pop_char() } -> std::same_as<void>;
};


template<typename T_node>
struct Array {
    using T_val = T_node::T_value;
    using T_this = Array<T_node>;

    template<typename Container>
    explicit Array(const Container &arr)
        : Array{arr.begin(), arr.end()}
    {}

    template<std::random_access_iterator T_it>
    Array(T_it begin, T_it end)
        : root{T_node::from_iter(begin, end)}
    {}

    ~Array() noexcept {
        delete root;
    }

    [[nodiscard]] bool empty() const { return !root; };

    [[nodiscard]] size_t size() const  { return root ? root->size() : 0; };

    template<typename T = T_node>
    [[nodiscard]] const T_node *operator[](size_t index) const
        requires IndexableConcept<T>
    {
        return root->from_index(index);
    }

    template<typename T = T_node>
    T_node *operator[](size_t index)
        requires IndexableConcept<T>
    {
        return root->from_index(index);
    }

    template<typename T = T_node>
    [[nodiscard]] const T_node *from_char_index(size_t idx) const
        requires LineConcept<T>
    {
        if(!root)
            return nullptr;
        return root->from_char_index(idx);
    }

    template<typename T = T_node>
    [[nodiscard]] T_node *from_char_index(size_t idx)
    requires LineConcept<T>
    {
        return const_cast<T_node *>(const_cast<const T_this *>(this)->from_char_index(idx));
    }

    void insert_after(T_node *it, T_val value) {
        if(!it->right())
            it->attach_tree(new T_node{value}, 1);
        else {
            it = it->right();
            while (it->left())
                it = it->left();
            it->attach_tree(new T_node{value}, 0);
        }

        root = fix(it);
    }

    void insert(size_t index, T_val value) {
        if(!root) {
            if(index != 0)
                throw std::out_of_range("Out of range.");
            root = new T_node{value};
            return;
        }

        T_node *it;
        if(index == 0) {
            it = begin();
            it->attach_tree(new T_node{value}, 0);
            root = fix(it);
        }
        else {
            it = root->from_index(--index);
            insert_after(it, value);
        }
    }

    T_val erase(T_node *it) {
        T_node *start;
        if(!it->left() && !it->right()) {
            start = it->parent;
            if(start)
                start->change_child(it, nullptr);
        }
        else if(!it->left() || !it->right()) {
            start = it->child[it->left() == nullptr];
            start->get_parent_from(it);
        }
        else if(!it->right()->left()) {
            start = it->right();
            start->get_parent_from(it);
            start->attach_tree(it->left(), 0);
        }
        else {
            T_node *next = it->right();
            while(next->left())
                next = next->left();

            start = next->parent;
            start->change_child(next, next->right());

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

    T_val erase(size_t index) {
        if(!root)
            throw std::out_of_range("Empty array.");

        T_node *it = root->from_index(index);
        return erase(it);
    }

private:
    [[nodiscard]] T_node *fix(T_node *current) {
        T_node *prev;
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

            T_node* (T_this::*rotation)(T_node*, T_node *);
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

    [[nodiscard]] T_node * RL_rot(T_node *x, T_node *y) {
        return L_rot(x, R_rot(y, y->child[0]));
    }

    [[nodiscard]] T_node * LR_rot(T_node *x, T_node *y) {
        return R_rot(x ,L_rot(y, y->child[1]));
    }

    [[nodiscard]] T_node * L_rot(T_node *x, T_node *y) {
        y->get_parent_from(x);
        x->attach_tree(y->child[0], 1);
        y->attach_tree(x, 0);
        return y;
    }

    [[nodiscard]] T_node * R_rot(T_node *x, T_node *y) {
        y->get_parent_from(x);
        x->attach_tree(y->child[1], 0);
        y->attach_tree(x, 1);
        return y;
    }

    T_node *begin() {
        if(!root) return nullptr;
        auto c = root;
        while(c->left()) c = c->left();
        return c;
    }

    T_node *root = nullptr;
};



struct TextEditorBackend {
    explicit TextEditorBackend(const std::string &text)
        : _text{text.begin(), text.end()},
          _lines{count_lengths(text)}
    {}

    [[nodiscard]] size_t size() const {
        return _text.size();
    }

    [[nodiscard]] size_t lines() const {
        return _lines.size();
    }

    char at(size_t i) const {
        return _text[i]->value;
    }

    void edit(size_t i, char c) {
        char &old = _text[i]->value;

        if(c == '\n' && old != '\n') {
            // split into two lines
            auto line_node = _lines.from_char_index(i);
            auto line_start = line_node->start();
            auto line_len = line_node->value;

            auto new_len1 = i - line_start + 1;
            auto new_len2 = line_len - new_len1;

            line_node->update_value(new_len1);
            _lines.insert_after(line_node, new_len2);
        }
        else if(old == '\n' && c != '\n') {
            // join two lines
            auto line_node = _lines.from_char_index(i);
            auto next_node = line_node->next();

            auto line_len = _lines.erase(next_node);
            line_node->add_chars(line_len);
        }

        old = c;
    }

    void insert(size_t i, char c) {
        auto line_node = _lines.from_char_index((i == size() && i > 0) ? i - 1 : i);
        _text.insert(i, (c == '\n') ? '\0' : c);
        line_node->add_char();

        if(c == '\n')
            edit(i, '\n');
    }

    void erase(size_t i) {
        edit(i, '\0');

        _text.erase(i);
        _lines.from_char_index(i)->pop_char();
    }

    size_t line_start(size_t r) const {
        return _lines[r]->start();
    }

    size_t line_length(size_t r) const {
        return _lines[r]->value;
    }

    size_t char_to_line(size_t i) const {
        if(i == _text.size())
            throw std::out_of_range("Out of range.");
        return _lines.from_char_index(i)->index();
    }

private:
    static std::vector<size_t> count_lengths(const std::string &text) {
        std::vector<size_t> lengths;
        size_t len = 0;
        for(auto c: text)
            if(len++, c == '\n')
                lengths.push_back(len), len = 0;

        lengths.push_back(len);
        return lengths;
    }

    Array<IndexableNode<char>> _text;
    Array<LinesNode<size_t>> _lines;
};

#ifndef __PROGTEST__

////////////////// Dark magic, ignore ////////////////////////

template<typename T>
auto quote(const T &t) { return t; }

std::string quote(const std::string &s) {
    std::string ret = "\"";
    for (char c: s) if (c != '\n') ret += c; else ret += "\\n";
    return ret + "\"";
}

#define STR_(a) #a
#define STR(a) STR_(a)

#define CHECK_(a, b, a_str, b_str) do { \
    auto _a = (a); \
    decltype(a) _b = (b); \
    if (_a != _b) { \
      std::cout << "Line " << __LINE__ << ": Assertion " \
        << a_str << " == " << b_str << " failed!" \
        << " (lhs: " << quote(_a) << ")" << std::endl; \
      fail++; \
    } else ok++; \
  } while (0)

#define CHECK(a, b) CHECK_(a, b, #a, #b)

#define CHECK_ALL(expr, ...) do { \
    std::array _arr = { __VA_ARGS__ }; \
    for (size_t _i = 0; _i < _arr.size(); _i++) \
      CHECK_((expr)(_i), _arr[_i], STR(expr) "(" << _i << ")", _arr[_i]); \
  } while (0)

#define CHECK_EX(expr, ex) do { \
    try { \
      (expr); \
      fail++; \
      std::cout << "Line " << __LINE__ << ": Expected " STR(expr) \
        " to throw " #ex " but no exception was raised." << std::endl; \
    } catch (const ex&) { ok++; \
    } catch (...) { \
      fail++; \
      std::cout << "Line " << __LINE__ << ": Expected " STR(expr) \
        " to throw " #ex " but got different exception." << std::endl; \
    } \
  } while (0)

////////////////// End of dark magic ////////////////////////


std::string text(const TextEditorBackend &t) {
    std::string ret;
    for (size_t i = 0; i < t.size(); i++) ret.push_back(t.at(i));
    return ret;
}

void test1(int &ok, int &fail) {
    TextEditorBackend s("123\n456\n789");
    CHECK(s.size(), 11);
    CHECK(text(s), "123\n456\n789");
    CHECK(s.lines(), 3);
    CHECK_ALL(s.char_to_line, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2);
    CHECK_ALL(s.line_start, 0, 4, 8);
    CHECK_ALL(s.line_length, 4, 4, 3);
}

void test2(int &ok, int &fail) {
    TextEditorBackend t("123\n456\n789\n");
    CHECK(t.size(), 12);
    CHECK(text(t), "123\n456\n789\n");
    CHECK(t.lines(), 4);
    CHECK_ALL(t.char_to_line, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2);
    CHECK_ALL(t.line_start, 0, 4, 8, 12);
    CHECK_ALL(t.line_length, 4, 4, 4, 0);
}

void test3(int &ok, int &fail) {
    TextEditorBackend t("asdfasdfasdf");

    CHECK(t.size(), 12);
    CHECK(text(t), "asdfasdfasdf");
    CHECK(t.lines(), 1);
    CHECK_ALL(t.char_to_line, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    CHECK(t.line_start(0), 0);
    CHECK(t.line_length(0), 12);

    t.insert(0, '\n');
    CHECK(t.size(), 13);
    CHECK(text(t), "\nasdfasdfasdf");
    CHECK(t.lines(), 2);
    CHECK_ALL(t.line_start, 0, 1);

    t.insert(4, '\n');
    CHECK(t.size(), 14);
    CHECK(text(t), "\nasd\nfasdfasdf");
    CHECK(t.lines(), 3);
    CHECK_ALL(t.line_start, 0, 1, 5);

    t.insert(t.size(), '\n');
    CHECK(t.size(), 15);
    CHECK(text(t), "\nasd\nfasdfasdf\n");
    CHECK(t.lines(), 4);
    CHECK_ALL(t.line_start, 0, 1, 5, 15);

    t.edit(t.size() - 1, 'H');
    CHECK(t.size(), 15);
    CHECK(text(t), "\nasd\nfasdfasdfH");
    CHECK(t.lines(), 3);
    CHECK_ALL(t.line_start, 0, 1, 5);

    t.erase(8);
    CHECK(t.size(), 14);
    CHECK(text(t), "\nasd\nfasfasdfH");
    CHECK(t.lines(), 3);
    CHECK_ALL(t.line_start, 0, 1, 5);

    t.erase(4);
    CHECK(t.size(), 13);
    CHECK(text(t), "\nasdfasfasdfH");
    CHECK(t.lines(), 2);
    CHECK_ALL(t.line_start, 0, 1);
}

void test_ex(int &ok, int &fail) {
    TextEditorBackend t("123\n456\n789\n");
    CHECK_EX(t.at(12), std::out_of_range);

    CHECK_EX(t.insert(13, 'a'), std::out_of_range);
    CHECK_EX(t.edit(12, 'x'), std::out_of_range);
    CHECK_EX(t.erase(12), std::out_of_range);

    CHECK_EX(t.line_start(4), std::out_of_range);
    CHECK_EX(t.line_start(40), std::out_of_range);
    CHECK_EX(t.line_length(4), std::out_of_range);
    CHECK_EX(t.line_length(6), std::out_of_range);
    CHECK_EX(t.char_to_line(12), std::out_of_range);
    CHECK_EX(t.char_to_line(25), std::out_of_range);
}

void test4(int &ok, int &fail) {
    TextEditorBackend t("");
    CHECK(text(t), "");
    CHECK(t.size(), 0);
    CHECK(t.lines(), 1);
    CHECK_ALL(t.line_start, 0);
    CHECK_ALL(t.line_length, 0);

    t.insert(0, '\n');
    CHECK(text(t), "\n");
    CHECK(t.size(), 1);
    CHECK(t.lines(), 2);
    CHECK_ALL(t.line_start, 0, 1);
    CHECK_ALL(t.line_length, 1, 0);
    CHECK_ALL(t.char_to_line, 0);

    t.insert(0, '\n');
    t.insert(1, '\n');
    t.insert(1, '\n');
    CHECK(text(t), "\n\n\n\n");
    CHECK(t.size(), 4);
    CHECK(t.lines(), 5);
    CHECK_ALL(t.line_start, 0, 1, 2, 3, 4);
    CHECK_ALL(t.line_length, 1, 1, 1, 1, 0);
    CHECK_ALL(t.char_to_line, 0, 1, 2, 3);

    t.erase(0);
    t.erase(1);
    t.erase(1);
    t.erase(0);
    CHECK(text(t), "");
    CHECK(t.size(), 0);
    CHECK(t.lines(), 1);
    CHECK_ALL(t.line_start, 0);
    CHECK_ALL(t.line_length, 0);

    t.insert(0, '\n');
    CHECK(text(t), "\n");
    CHECK(t.size(), 1);
    CHECK(t.lines(), 2);
    CHECK_ALL(t.line_start, 0, 1);
    CHECK_ALL(t.line_length, 1, 0);
    CHECK_ALL(t.char_to_line, 0);
}

int main() {
    int ok = 0, fail = 0;
    if (!fail) test1(ok, fail);
    if (!fail) test2(ok, fail);
    if (!fail) test3(ok, fail);
    if (!fail) test4(ok, fail);
    if (!fail) test_ex(ok, fail);

    if (!fail) std::cout << "Passed all " << ok << " tests!" << std::endl;
    else std::cout << "Failed " << fail << " of " << (ok + fail) << " tests." << std::endl;
}

#endif


