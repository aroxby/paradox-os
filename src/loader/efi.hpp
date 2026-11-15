#ifndef WORKSPACES_PARADOX_OS_SRC_LOADER_EFI_HPP
#define WORKSPACES_PARADOX_OS_SRC_LOADER_EFI_HPP

#include <efi.h>
#include <efilib.h>

#define EFI_STR(Str) (reinterpret_cast<const CHAR16 *>(u##Str))

void *operator new(size_t size) { return AllocatePool(size); }

void *operator new[](unsigned long size) { return AllocatePool(size); }

void operator delete(void *ptr) { return FreePool(ptr); }

void operator delete[](void *ptr) { return FreePool(ptr); }

#endif // WORKSPACES_PARADOX_OS_SRC_LOADER_EFI_HPP
