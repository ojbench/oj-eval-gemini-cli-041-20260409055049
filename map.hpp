#ifndef SJTU_MAP_HPP
#define SJTU_MAP_HPP

#include <functional>
#include <cstddef>
#include "utility.hpp"
#include "exceptions.hpp"

namespace sjtu {

template<class Key, class T, class Compare = std::less<Key>>
class map {
public:
    typedef pair<const Key, T> value_type;

private:
    struct NodeBase {
        NodeBase *left, *right, *parent;
        int height;
        NodeBase() : left(nullptr), right(nullptr), parent(nullptr), height(1) {}
    };

    struct Node : public NodeBase {
        alignas(value_type) char data[sizeof(value_type)];
        value_type* get_data() {
            return reinterpret_cast<value_type*>(data);
        }
        const value_type* get_data() const {
            return reinterpret_cast<const value_type*>(data);
        }
    };

    NodeBase *header;
    size_t tree_size;
    Compare comp;

    NodeBase*& root() { return header->parent; }
    NodeBase* const& root() const { return header->parent; }
    NodeBase*& leftmost() { return header->left; }
    NodeBase* const& leftmost() const { return header->left; }
    NodeBase*& rightmost() { return header->right; }
    NodeBase* const& rightmost() const { return header->right; }

    int get_height(NodeBase *node) const {
        return node ? node->height : 0;
    }

    void update_height(NodeBase *node) {
        if (node) {
            int lh = get_height(node->left);
            int rh = get_height(node->right);
            node->height = (lh > rh ? lh : rh) + 1;
        }
    }

    NodeBase* minimum(NodeBase *node) const {
        while (node->left) node = node->left;
        return node;
    }

    NodeBase* maximum(NodeBase *node) const {
        while (node->right) node = node->right;
        return node;
    }

    void rotate_left(NodeBase *x) {
        NodeBase *y = x->right;
        x->right = y->left;
        if (y->left) y->left->parent = x;
        y->parent = x->parent;
        if (x == root()) {
            root() = y;
        } else if (x == x->parent->left) {
            x->parent->left = y;
        } else {
            x->parent->right = y;
        }
        y->left = x;
        x->parent = y;
        update_height(x);
        update_height(y);
    }

    void rotate_right(NodeBase *x) {
        NodeBase *y = x->left;
        x->left = y->right;
        if (y->right) y->right->parent = x;
        y->parent = x->parent;
        if (x == root()) {
            root() = y;
        } else if (x == x->parent->right) {
            x->parent->right = y;
        } else {
            x->parent->left = y;
        }
        y->right = x;
        x->parent = y;
        update_height(x);
        update_height(y);
    }

    void rebalance(NodeBase *node) {
        while (node != header) {
            update_height(node);
            int lh = get_height(node->left);
            int rh = get_height(node->right);
            if (lh - rh > 1) {
                int llh = get_height(node->left->left);
                int lrh = get_height(node->left->right);
                if (llh >= lrh) {
                    rotate_right(node);
                } else {
                    rotate_left(node->left);
                    rotate_right(node);
                }
                node = node->parent; // after rotation, node is pushed down
            } else if (rh - lh > 1) {
                int rlh = get_height(node->right->left);
                int rrh = get_height(node->right->right);
                if (rrh >= rlh) {
                    rotate_left(node);
                } else {
                    rotate_right(node->right);
                    rotate_left(node);
                }
                node = node->parent;
            }
            node = node->parent;
        }
    }

    Node* create_node(const value_type &val) {
        Node *node = new Node();
        new (node->data) value_type(val);
        return node;
    }

    void destroy_node(Node *node) {
        node->get_data()->~value_type();
        delete node;
    }

    void clear_tree(NodeBase *node) {
        if (!node) return;
        clear_tree(node->left);
        clear_tree(node->right);
        destroy_node(static_cast<Node*>(node));
    }

    NodeBase* copy_tree(NodeBase *node, NodeBase *parent) {
        if (!node) return nullptr;
        Node *new_node = create_node(*static_cast<Node*>(node)->get_data());
        new_node->parent = parent;
        new_node->height = node->height;
        new_node->left = copy_tree(node->left, new_node);
        new_node->right = copy_tree(node->right, new_node);
        return new_node;
    }

public:
    class iterator;
    class const_iterator;

    class iterator {
        friend class map;
    private:
        NodeBase *node;
        map *m;
    public:
        iterator() : node(nullptr), m(nullptr) {}
        iterator(NodeBase *n, map *m) : node(n), m(m) {}
        iterator(const iterator &other) : node(other.node), m(other.m) {}

        iterator operator++(int) {
            iterator tmp = *this;
            ++*this;
            return tmp;
        }

        iterator& operator++() {
            if (node == m->header) throw invalid_iterator();
            if (node->right) {
                node = node->right;
                while (node->left) node = node->left;
            } else {
                NodeBase *p = node->parent;
                while (p != m->header && node == p->right) {
                    node = p;
                    p = p->parent;
                }
                node = p;
            }
            return *this;
        }

        iterator operator--(int) {
            iterator tmp = *this;
            --*this;
            return tmp;
        }

        iterator& operator--() {
            if (node == m->header) {
                if (m->tree_size == 0) throw invalid_iterator();
                node = m->rightmost();
            } else if (node->left) {
                node = node->left;
                while (node->right) node = node->right;
            } else {
                NodeBase *p = node->parent;
                while (p != m->header && node == p->left) {
                    node = p;
                    p = p->parent;
                }
                if (p == m->header) throw invalid_iterator();
                node = p;
            }
            return *this;
        }

        value_type& operator*() const {
            if (node == m->header) throw invalid_iterator();
            return *static_cast<Node*>(node)->get_data();
        }

        value_type* operator->() const {
            if (node == m->header) throw invalid_iterator();
            return static_cast<Node*>(node)->get_data();
        }

        bool operator==(const iterator &rhs) const { return node == rhs.node; }
        bool operator==(const const_iterator &rhs) const { return node == rhs.node; }
        bool operator!=(const iterator &rhs) const { return node != rhs.node; }
        bool operator!=(const const_iterator &rhs) const { return node != rhs.node; }
    };

    class const_iterator {
        friend class map;
    private:
        const NodeBase *node;
        const map *m;
    public:
        const_iterator() : node(nullptr), m(nullptr) {}
        const_iterator(const NodeBase *n, const map *m) : node(n), m(m) {}
        const_iterator(const const_iterator &other) : node(other.node), m(other.m) {}
        const_iterator(const iterator &other) : node(other.node), m(other.m) {}

        const_iterator operator++(int) {
            const_iterator tmp = *this;
            ++*this;
            return tmp;
        }

        const_iterator& operator++() {
            if (node == m->header) throw invalid_iterator();
            if (node->right) {
                node = node->right;
                while (node->left) node = node->left;
            } else {
                const NodeBase *p = node->parent;
                while (p != m->header && node == p->right) {
                    node = p;
                    p = p->parent;
                }
                node = p;
            }
            return *this;
        }

        const_iterator operator--(int) {
            const_iterator tmp = *this;
            --*this;
            return tmp;
        }

        const_iterator& operator--() {
            if (node == m->header) {
                if (m->tree_size == 0) throw invalid_iterator();
                node = m->rightmost();
            } else if (node->left) {
                node = node->left;
                while (node->right) node = node->right;
            } else {
                const NodeBase *p = node->parent;
                while (p != m->header && node == p->left) {
                    node = p;
                    p = p->parent;
                }
                if (p == m->header) throw invalid_iterator();
                node = p;
            }
            return *this;
        }

        const value_type& operator*() const {
            if (node == m->header) throw invalid_iterator();
            return *static_cast<const Node*>(node)->get_data();
        }

        const value_type* operator->() const {
            if (node == m->header) throw invalid_iterator();
            return static_cast<const Node*>(node)->get_data();
        }

        bool operator==(const iterator &rhs) const { return node == rhs.node; }
        bool operator==(const const_iterator &rhs) const { return node == rhs.node; }
        bool operator!=(const iterator &rhs) const { return node != rhs.node; }
        bool operator!=(const const_iterator &rhs) const { return node != rhs.node; }
    };

    map() {
        header = new NodeBase();
        header->parent = nullptr;
        header->left = header;
        header->right = header;
        tree_size = 0;
    }

    map(const map &other) {
        header = new NodeBase();
        header->parent = nullptr;
        header->left = header;
        header->right = header;
        tree_size = other.tree_size;
        comp = other.comp;
        if (other.root()) {
            root() = copy_tree(other.root(), header);
            leftmost() = minimum(root());
            rightmost() = maximum(root());
        }
    }

    map& operator=(const map &other) {
        if (this == &other) return *this;
        clear();
        comp = other.comp;
        tree_size = other.tree_size;
        if (other.root()) {
            root() = copy_tree(other.root(), header);
            leftmost() = minimum(root());
            rightmost() = maximum(root());
        }
        return *this;
    }

    ~map() {
        clear();
        delete header;
    }

    T& at(const Key &key) {
        NodeBase *node = root();
        while (node) {
            const Key &k = static_cast<Node*>(node)->get_data()->first;
            if (comp(key, k)) {
                node = node->left;
            } else if (comp(k, key)) {
                node = node->right;
            } else {
                return static_cast<Node*>(node)->get_data()->second;
            }
        }
        throw index_out_of_bound();
    }

    const T& at(const Key &key) const {
        NodeBase *node = root();
        while (node) {
            const Key &k = static_cast<const Node*>(node)->get_data()->first;
            if (comp(key, k)) {
                node = node->left;
            } else if (comp(k, key)) {
                node = node->right;
            } else {
                return static_cast<const Node*>(node)->get_data()->second;
            }
        }
        throw index_out_of_bound();
    }

    T& operator[](const Key &key) {
        NodeBase *node = root();
        NodeBase *parent = header;
        bool is_left = true;
        while (node) {
            parent = node;
            const Key &k = static_cast<Node*>(node)->get_data()->first;
            if (comp(key, k)) {
                is_left = true;
                node = node->left;
            } else if (comp(k, key)) {
                is_left = false;
                node = node->right;
            } else {
                return static_cast<Node*>(node)->get_data()->second;
            }
        }
        Node *new_node = create_node(value_type(key, T()));
        new_node->parent = parent;
        if (parent == header) {
            root() = new_node;
            leftmost() = new_node;
            rightmost() = new_node;
        } else if (is_left) {
            parent->left = new_node;
            if (parent == leftmost()) leftmost() = new_node;
        } else {
            parent->right = new_node;
            if (parent == rightmost()) rightmost() = new_node;
        }
        tree_size++;
        rebalance(new_node);
        return new_node->get_data()->second;
    }

    const T& operator[](const Key &key) const {
        return at(key);
    }

    iterator begin() { return iterator(leftmost(), this); }
    const_iterator begin() const { return const_iterator(leftmost(), this); }
    const_iterator cbegin() const { return const_iterator(leftmost(), this); }

    iterator end() { return iterator(header, this); }
    const_iterator end() const { return const_iterator(header, this); }
    const_iterator cend() const { return const_iterator(header, this); }

    bool empty() const { return tree_size == 0; }
    size_t size() const { return tree_size; }

    void clear() {
        clear_tree(root());
        root() = nullptr;
        leftmost() = header;
        rightmost() = header;
        tree_size = 0;
    }

    pair<iterator, bool> insert(const value_type &value) {
        NodeBase *node = root();
        NodeBase *parent = header;
        bool is_left = true;
        while (node) {
            parent = node;
            const Key &k = static_cast<Node*>(node)->get_data()->first;
            if (comp(value.first, k)) {
                is_left = true;
                node = node->left;
            } else if (comp(k, value.first)) {
                is_left = false;
                node = node->right;
            } else {
                return pair<iterator, bool>(iterator(node, this), false);
            }
        }
        Node *new_node = create_node(value);
        new_node->parent = parent;
        if (parent == header) {
            root() = new_node;
            leftmost() = new_node;
            rightmost() = new_node;
        } else if (is_left) {
            parent->left = new_node;
            if (parent == leftmost()) leftmost() = new_node;
        } else {
            parent->right = new_node;
            if (parent == rightmost()) rightmost() = new_node;
        }
        tree_size++;
        rebalance(new_node);
        return pair<iterator, bool>(iterator(new_node, this), true);
    }

    void erase(iterator pos) {
        if (pos.node == header || pos.node == nullptr) throw invalid_iterator();
        NodeBase *node = pos.node;
        NodeBase *y = (node->left == nullptr || node->right == nullptr) ? node : (++iterator(node, this)).node;
        NodeBase *x = y->left ? y->left : y->right;
        NodeBase *x_parent = y->parent;

        if (x) x->parent = y->parent;
        if (y->parent == header) {
            root() = x;
        } else if (y == y->parent->left) {
            y->parent->left = x;
        } else {
            y->parent->right = x;
        }

        if (y != node) {
            // replace node with y
            y->parent = node->parent;
            if (node->parent == header) {
                root() = y;
            } else if (node == node->parent->left) {
                node->parent->left = y;
            } else {
                node->parent->right = y;
            }
            y->left = node->left;
            if (y->left) y->left->parent = y;
            y->right = node->right;
            if (y->right) y->right->parent = y;
            y->height = node->height;
            if (x_parent == node) x_parent = y;
        }

        if (leftmost() == node) {
            leftmost() = root() ? minimum(root()) : header;
        }
        if (rightmost() == node) {
            rightmost() = root() ? maximum(root()) : header;
        }

        if (x_parent != header) {
            rebalance(x_parent);
        }

        destroy_node(static_cast<Node*>(node));
        tree_size--;
    }

    size_t count(const Key &key) const {
        NodeBase *node = root();
        while (node) {
            const Key &k = static_cast<const Node*>(node)->get_data()->first;
            if (comp(key, k)) {
                node = node->left;
            } else if (comp(k, key)) {
                node = node->right;
            } else {
                return 1;
            }
        }
        return 0;
    }

    iterator find(const Key &key) {
        NodeBase *node = root();
        while (node) {
            const Key &k = static_cast<Node*>(node)->get_data()->first;
            if (comp(key, k)) {
                node = node->left;
            } else if (comp(k, key)) {
                node = node->right;
            } else {
                return iterator(node, this);
            }
        }
        return end();
    }

    const_iterator find(const Key &key) const {
        NodeBase *node = root();
        while (node) {
            const Key &k = static_cast<const Node*>(node)->get_data()->first;
            if (comp(key, k)) {
                node = node->left;
            } else if (comp(k, key)) {
                node = node->right;
            } else {
                return const_iterator(node, this);
            }
        }
        return cend();
    }
};

}

#endif
