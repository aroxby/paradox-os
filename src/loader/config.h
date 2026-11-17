#ifndef WORKSPACES_PARADOX_OS_SRC_LOADER_CONFIG_H
#define WORKSPACES_PARADOX_OS_SRC_LOADER_CONFIG_H

#include "list.h"
#include "pair.h"
#include "string.h"

class Config : public List<Pair<const String, String>> {
  public:
    Iterator find(const String &key) const {
        Iterator it = begin();
        for (; it != end(); ++it) {
            if (it->first == key) {
                break;
            }
        }
        return it;
    }
};

#endif // WORKSPACES_PARADOX_OS_SRC_LOADER_CONFIG_H
