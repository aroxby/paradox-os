#ifndef WORKSPACES_PARADOX_OS_EFI_HPP
#define WORKSPACES_PARADOX_OS_EFI_HPP

#include <efi.h>
#include <efilib.h>

#define EFI_STR(str) (reinterpret_cast<const CHAR16 *>(u##str))

#endif // WORKSPACES_PARADOX_OS_EFI_HPP
