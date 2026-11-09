#ifndef WORKSPACES_PARADOX_OS_SRC_LOADER_RESULT_H
#define WORKSPACES_PARADOX_OS_SRC_LOADER_RESULT_H

#include <efi.h>
#include <expected>

template <typename T> using Result = std::expected<T, EFI_STATUS>;

#endif // WORKSPACES_PARADOX_OS_SRC_LOADER_RESULT_H
