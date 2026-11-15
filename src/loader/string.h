#ifndef WORKSPACES_PARADOX_OS_SRC_LOADER_STRING_H
#define WORKSPACES_PARADOX_OS_SRC_LOADER_STRING_H

#include <cstddef>
#include <cstdint>
#include <memory>

typedef uint16_t CHAR16;

size_t wcstrlen(const CHAR16 *str) {
    size_t len = 0;
    while (str[len] != 0) {
        len++;
    }
    return len;
}

void wcstrcpy(CHAR16 *dest, const CHAR16 *src) {
    size_t i = 0;
    while (src[i] != 0) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = 0;
}

CHAR16 *wcschr(CHAR16 *str, CHAR16 ch) {
    while (*str != 0) {
        if (*str == ch) {
            return str;
        }
        str++;
    }
    return nullptr;
}

CHAR16 *wcsstrip(CHAR16 *str) {
    while (*str == L' ' || *str == L'\t') {
        str++;
    }

    CHAR16 *end = str + wcstrlen(str);
    while (end > str && (*end == L' ' || *end == L'\t' || *end == 0)) {
        *end = 0;
        end--;
    }

    return str;
}

int wcstrcmp(const CHAR16 *str1, const CHAR16 *str2) {
    while (*str1 != 0 && *str1 == *str2) {
        str1++;
        str2++;
    }
    return *str1 - *str2;
}

class String {
  public:
    static constexpr size_t npos = static_cast<size_t>(-1);

    String() : buffer(new CHAR16[1]{0}) {}

    explicit(false) String(const CHAR16 *str) {
        buffer.reset(new CHAR16[wcstrlen(str) + 1]);
        wcstrcpy(buffer.get(), str);
    }

    String(const String &other) {
        buffer.reset(new CHAR16[other.length() + 1]);
        wcstrcpy(buffer.get(), other.buffer.get());
    }

    bool operator==(const String &other) const { return wcstrcmp(buffer.get(), other.buffer.get()) == 0; }

    const CHAR16 *c_str() const { return buffer.get(); }

    CHAR16 *data() { return buffer.get(); }

    size_t length() const { return wcstrlen(buffer.get()); }

    size_t find_last_of(CHAR16 ch) const {
        for (size_t i = length(); i > 0; i--) {
            if (buffer.get()[i - 1] == ch) {
                return i - 1;
            }
        }
        return npos;
    }

    void replace(size_t pos, size_t len, const CHAR16 *str) {
        size_t new_len = length() - len + wcstrlen(str);
        std::unique_ptr<CHAR16[]> new_buffer(new CHAR16[new_len + 1]);
        size_t i = 0;
        for (size_t j = 0; j < pos; j++) {
            new_buffer[i++] = buffer.get()[j];
        }
        for (size_t j = 0; j < wcstrlen(str); j++) {
            new_buffer[i++] = str[j];
        }
        for (size_t j = pos + len; j < length(); j++) {
            new_buffer[i++] = buffer.get()[j];
        }
        new_buffer[i] = 0;
        buffer = std::move(new_buffer);
    }

  private:
    std::unique_ptr<CHAR16[]> buffer;
};

#endif // WORKSPACES_PARADOX_OS_SRC_LOADER_STRING_H
