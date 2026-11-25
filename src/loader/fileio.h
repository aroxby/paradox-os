#ifndef WORKSPACES_PARADOX_OS_SRC_LOADER_FILEIO_H
#define WORKSPACES_PARADOX_OS_SRC_LOADER_FILEIO_H

#include "efi.hpp"
#include "result.h"
#include "string.h"

Result<String> loadFile(EFI_HANDLE imageHandle, const String &filePath);

#endif // WORKSPACES_PARADOX_OS_SRC_LOADER_FILEIO_H
