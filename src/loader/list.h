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
        explicit Iterator(Node *node) : current(node) {}
        T &operator*() const { return current->value; }
        T *operator->() const { return &current->value; }

        Iterator &operator++() {
            current = current->next.get();
            return *this;
        }
        Iterator operator++(int) {
            Iterator tmp = *this;
            current = current->next.get();
            return tmp;
        }

        bool operator!=(const Iterator &other) const { return current != other.current; }

      private:
        Node *current;
    };

    void insert(const T &value) {
        std::unique_ptr<Node> newNode(new Node{value, std::move(head)});
        head = std::move(newNode);
    }

    Iterator begin() { return Iterator(head.get()); }
    Iterator end() { return Iterator(tail.get()); }

    Iterator begin() const { return Iterator(head.get()); }
    Iterator end() const { return Iterator(tail.get()); }

  private:
    // Note: Tail is also owned by the nodes but is null
    std::unique_ptr<Node> head, tail;
};

#endif // WORKSPACES_PARADOX_OS_SRC_LOADER_LIST_H
