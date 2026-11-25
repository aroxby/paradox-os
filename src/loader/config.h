#ifndef WORKSPACES_PARADOX_OS_SRC_LOADER_CONFIG_H
#define WORKSPACES_PARADOX_OS_SRC_LOADER_CONFIG_H

#include "efi.hpp"
#include "map.h"
#include "string.h"

using ConfigArgs = Map<String, String>;

class Config {
  public:
    String kernelPath;
    EFI_GUID partitionUuid;
};

#endif // WORKSPACES_PARADOX_OS_SRC_LOADER_CONFIG_H
