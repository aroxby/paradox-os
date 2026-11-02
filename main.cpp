#include <efi.h>
#include <efilib.h>

#define EFI_STR(str) (reinterpret_cast<const CHAR16 *>(u##str))

static void qemuQuit(UINT8 Status) {
    // Writes the exit status to port 0xF4
    // QEMU translates a value of 'status' into a QEMU exit code of: (status << 1) | 1
    // If you pass 0, QEMU exits with status code 1.
    __asm__ __volatile__("outb %0, %1" : : "a"(Status), "Nd"(0xf4));
}

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
