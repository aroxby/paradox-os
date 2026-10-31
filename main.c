#include <efi.h>
#include <efilib.h>

void QemuQuit(UINT8 status) {
    // Writes the exit status to port 0xF4
    // QEMU translates a value of 'status' into a QEMU exit code of: (status << 1) | 1
    // If you pass 0, QEMU exits with status code 1.
    __asm__ __volatile__ ("outb %0, %1" : : "a"(status), "Nd"(0xf4));
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    // Initialize the gnu-efi helper library
    InitializeLib(ImageHandle, SystemTable);

    // Print a 16-bit wide Unicode string to the screen
    Print(L"Hello, UEFI!\n");

    QemuQuit(0);

    for(;;);

    return EFI_SUCCESS;
}
