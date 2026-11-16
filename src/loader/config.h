#ifndef WORKSPACES_PARADOX_OS_SRC_LOADER_CONFIG_H
#define WORKSPACES_PARADOX_OS_SRC_LOADER_CONFIG_H

#include "list.h"
#include "pair.h"
#include "string.h"

class Config : public List<Pair<String, String>> {
  private:
    // `static` is omitted here because it requires additional code not present with -nostdlib
    String empty;

  public:
    const String &find(const String &key) const {
        for (auto &value : *this) {
            if (value.first == key) {
                return value.second;
            }
        }
        // Returning the empty string turned out to be pain here.
        // TODO: We should insert it (like std::map::operator[]) or return an iterator (like std::map::find).
        return empty;
    }
};

#endif // WORKSPACES_PARADOX_OS_SRC_LOADER_CONFIG_H
