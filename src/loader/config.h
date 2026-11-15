#ifndef WORKSPACES_PARADOX_OS_SRC_LOADER_CONFIG_H
#define WORKSPACES_PARADOX_OS_SRC_LOADER_CONFIG_H

#include "list.h"
#include "pair.h"
#include "string.h"

class Config : public List<Pair<String, String>> {
    const String &find(const String &key) const {
        for (auto &value : *this) {
            if (value.first == key) {
                return value.second;
            }
        }
        static String empty;
        return empty;
    }
};

#endif // WORKSPACES_PARADOX_OS_SRC_LOADER_CONFIG_H
