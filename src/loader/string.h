#ifndef WORKSPACES_PARADOX_OS_SRC_LOADER_STRING_H
#define WORKSPACES_PARADOX_OS_SRC_LOADER_STRING_H

#include <cstddef>
#include <cstdint>
#include <memory>

typedef uint16_t CHAR16;

size_t wcslen(const CHAR16 *str);
void wcstrcpy(CHAR16 *dest, const CHAR16 *src);
CHAR16 *wcschr(CHAR16 *str, CHAR16 ch);
CHAR16 *wcsstrip(CHAR16 *str);
int wcstrcmp(const CHAR16 *str1, const CHAR16 *str2);

class String {
  public:
    static constexpr size_t npos = static_cast<size_t>(-1);

    String() : buffer(new CHAR16[1]{0}) {}

    explicit(false) String(const CHAR16 *str) {
        buffer.reset(new CHAR16[wcslen(str) + 1]);
        wcstrcpy(buffer.get(), str);
    }

    String(const String &other) {
        buffer.reset(new CHAR16[other.length() + 1]);
        wcstrcpy(buffer.get(), other.buffer.get());
    }

    bool operator==(const String &other) const { return wcstrcmp(buffer.get(), other.buffer.get()) == 0; }

    const CHAR16 &operator[](size_t index) const { return buffer.get()[index]; }

    const CHAR16 *c_str() const { return buffer.get(); }

    CHAR16 *data() { return buffer.get(); }

    size_t length() const { return wcslen(buffer.get()); }

    size_t find_last_of(CHAR16 ch) const;

    void replace(size_t pos, size_t len, const CHAR16 *str);

  private:
    std::unique_ptr<CHAR16[]> buffer;
};

#endif // WORKSPACES_PARADOX_OS_SRC_LOADER_STRING_H
