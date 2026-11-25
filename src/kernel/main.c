#include <efi.h>
#include <efilib.h>

#define EFI_STR(str) ((const CHAR16 *)(u##str))

EFI_STATUS EFIAPI efi_main(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE *systemTable) {
    InitializeLib(imageHandle, systemTable);
    Print(EFI_STR("Kernel panic! You have no kernel!\n"));
    return EFI_ABORTED;
}
