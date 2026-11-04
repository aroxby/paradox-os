#ifndef WORKSPACES_PARADOX_OS_EFI_HPP
#define WORKSPACES_PARADOX_OS_EFI_HPP

#include <efi.h>
#include <efilib.h>

#define EFI_STR(str) (reinterpret_cast<const CHAR16 *>(u##str))

inline void *operator new(size_t size) { return AllocatePool(size); }

inline void *operator new[](unsigned long size) { return AllocatePool(size); }

inline void operator delete(void *ptr) { return FreePool(ptr); }

inline void operator delete[](void *ptr) { return FreePool(ptr); }

#endif // WORKSPACES_PARADOX_OS_EFI_HPP
