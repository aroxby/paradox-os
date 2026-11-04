#ifndef WORKSPACES_PARADOX_OS_SRC_LOADER_EFI_HPP
#define WORKSPACES_PARADOX_OS_SRC_LOADER_EFI_HPP

#include <efi.h>
#include <efilib.h>

#define EFI_STR(Str) (reinterpret_cast<const CHAR16 *>(u##Str))

inline void *operator new(size_t Size) { return AllocatePool(Size); }

inline void *operator new[](unsigned long Size) { return AllocatePool(Size); }

inline void operator delete(void *Ptr) { return FreePool(Ptr); }

inline void operator delete[](void *Ptr) { return FreePool(Ptr); }

#endif // WORKSPACES_PARADOX_OS_SRC_LOADER_EFI_HPP
