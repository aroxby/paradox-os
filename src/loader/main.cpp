#include "efi.hpp"
#include "qemu.h"

extern "C" EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    // Initialize the gnu-efi helper library
    InitializeLib(ImageHandle, SystemTable);

    // Print a 16-bit wide Unicode string to the screen
    Print(EFI_STR("Hello, UEFI!\n"));

    qemuQuit(0);

    for (;;) {
    }

    return EFI_SUCCESS;
}
