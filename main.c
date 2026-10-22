#include <efi.h>
#include <efilib.h>

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    // Initialize the gnu-efi helper library
    InitializeLib(ImageHandle, SystemTable);

    // Print a 16-bit wide Unicode string to the screen
    Print(L"Hello, UEFI!\n");

    for(;;);

    return EFI_SUCCESS;
}
