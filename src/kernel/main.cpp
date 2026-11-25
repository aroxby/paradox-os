#include "efi.hpp"

extern "C" EFI_STATUS EFIAPI efi_main(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE *systemTable) {
    InitializeLib(imageHandle, systemTable);
    Print(EFI_STR("Kernel panic! You have no kernel!\n"));
    return EFI_ABORTED;
}
