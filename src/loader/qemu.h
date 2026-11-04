#ifndef WORKSPACES_PARADOX_OS_SRC_LOADER_QEMU_H
#define WORKSPACES_PARADOX_OS_SRC_LOADER_QEMU_H

inline static void qemuQuit(unsigned char Status) {
    // Writes the exit status to port 0xF4
    // QEMU translates a value of 'status' into a QEMU exit code of: (status << 1) | 1
    // If you pass 0, QEMU exits with status code 1.
    __asm__ __volatile__("outb %0, %1" : : "a"(Status), "Nd"(0xf4));
}

#endif // WORKSPACES_PARADOX_OS_SRC_LOADER_QEMU_H
