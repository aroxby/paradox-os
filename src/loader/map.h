#ifndef WORKSPACES_PARADOX_OS_SRC_LOADER_MAP_H
#define WORKSPACES_PARADOX_OS_SRC_LOADER_MAP_H

#include "list.h"
#include "pair.h"

template <typename Key, typename Value> class Map : public List<Pair<const Key, Value>> {
  public:
    using Base = List<Pair<const Key, Value>>;

    const typename Base::Iterator find(const Key &key) const {
        auto it = Base::begin();
        for (; it != Base::end(); ++it) {
            if (it->first == key) {
                break;
            }
        }
        return it;
    }
};

#endif // WORKSPACES_PARADOX_OS_SRC_LOADER_MAP_H
