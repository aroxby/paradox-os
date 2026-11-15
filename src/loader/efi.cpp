#include "efi.hpp"

void *operator new(size_t size) { return AllocatePool(size); }

void *operator new[](unsigned long size) { return AllocatePool(size); }

void operator delete(void *ptr) noexcept { return FreePool(ptr); }

void operator delete[](void *ptr) noexcept { return FreePool(ptr); }

void operator delete(void *ptr, size_t) noexcept { return FreePool(ptr); }
