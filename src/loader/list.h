#ifndef WORKSPACES_PARADOX_OS_SRC_LOADER_LIST_H
#define WORKSPACES_PARADOX_OS_SRC_LOADER_LIST_H

#include <memory>

template <typename T> class List {
  private:
    class Node {
      public:
        T value;
        std::unique_ptr<Node> next;
    };

  public:
    class Iterator {
      public:
        explicit Iterator(Node *node) : value(&node->value), next(node->next.get()) {}
        T &operator*() const { return *value; }
        T *operator->() const { return value; }

        Iterator &operator++() {
            inc();
            return *this;
        }
        Iterator operator++(int) {
            Iterator tmp = *this;
            inc();
            return tmp;
        }

        bool operator!=(const Iterator &other) const { return next != other.next || value != other.value; }

      private:
        T *value;
        Node *next;

        void inc() {
            value = &next->value;
            next = next->next.get();
        }
    };

    void insert(const T &value) {
        std::unique_ptr<Node> newNode(new Node{value, std::move(head)});
        head = std::move(newNode);
    }

    Iterator begin() { return Iterator(head.get()); }
    Iterator end() { return Iterator(tail.get()); }

  private:
    // Note: Tail is also owned by the nodes but is null
    std::unique_ptr<Node> head, tail;
};

#endif // WORKSPACES_PARADOX_OS_SRC_LOADER_LIST_H
