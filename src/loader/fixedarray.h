#ifndef WORKSPACES_PARADOX_OS_SRC_LOADER_FIXEDARRAY_H
#define WORKSPACES_PARADOX_OS_SRC_LOADER_FIXEDARRAY_H

#include <cstddef>
#include <memory>

template <typename T> class FixedArray {
  private:
    std::unique_ptr<T[]> m_data;
    size_t m_size;

  public:
    FixedArray() : m_data(nullptr), m_size(0) {}

    explicit FixedArray(size_t size) : m_data(new T[size]()), m_size(size) {}

    explicit FixedArray(T *data, size_t size) : m_data(data), m_size(size) {}

    T &operator[](size_t index) { return m_data[index]; }
    const T &operator[](size_t index) const { return m_data[index]; }

    size_t size() const noexcept { return m_size; }

    T *begin() noexcept { return m_data.get(); }
    T *end() noexcept { return m_data.get() + m_size; }
    const T *begin() const noexcept { return m_data.get(); }
    const T *end() const noexcept { return m_data.get() + m_size; }
};

#endif // WORKSPACES_PARADOX_OS_SRC_LOADER_FIXEDARRAY_H
