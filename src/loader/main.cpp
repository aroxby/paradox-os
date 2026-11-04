#include "efi.hpp"
#include "qemu.h"

#include <memory>

static uint8_t fatIdOffset(const uint8_t *Sector0) {
    // Generic FAT signature
    if (Sector0[0] != 0xEB && Sector0[0] != 0xE9) {
        return 0;
    }
    if (*(uint16_t *)(Sector0 + 0x1FE) != 0xAA55) {
        return 0;
    }

    // FAT12 ("FAT12   ")
    if (*(uint64_t *)(Sector0 + 0x36) == 0x2020203231544146) {
        return 0x27;
    }

    // FAT16 ("FAT16   ")
    if (*(uint64_t *)(Sector0 + 0x36) == 0x2020203631544146) {
        return 0x27;
    }

    // FAT32 ("FAT32   ")
    if (*(uint64_t *)(Sector0 + 0x52) == 0x2020203233544146) {
        return 0x43;
    }

    return 0;
}

static EFI_STATUS getVolumeId(EFI_HANDLE ImageHandle, EFI_HANDLE FsHandle, UINT32 *VolumeId) {
    EFI_STATUS Status;
    EFI_BLOCK_IO *BlockIo = NULL;

    // 1. Open the Block IO Protocol on the same handle as the File System
    Status = uefi_call_wrapper((void *)gBS->OpenProtocol, 6, FsHandle, &BlockIoProtocol, (VOID **)&BlockIo, ImageHandle,
                               NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);

    if (EFI_ERROR(Status)) {
        return Status;
    }

    // 2. Allocate a buffer matching the media's block size
    UINT32 BlockSize = BlockIo->Media->BlockSize;
    std::unique_ptr<UINT8[]> SectorBuffer(new UINT8[BlockSize]);
    if (!SectorBuffer) {
        return EFI_OUT_OF_RESOURCES;
    }

    // 3. Read LBA 0
    Status = uefi_call_wrapper((void *)BlockIo->ReadBlocks, 5, BlockIo, BlockIo->Media->MediaId, 0, BlockSize,
                               SectorBuffer.get());

    // 4. Extract FAT Volume Serial Number
    UINT8 IdOffset = fatIdOffset(SectorBuffer.get());
    if (IdOffset) {
        UINT32 serial = *(UINT32 *)(SectorBuffer.get() + IdOffset);
        *VolumeId = serial;
        return EFI_SUCCESS;
    }

    return EFI_UNSUPPORTED;
}

extern "C" EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);
    Print(EFI_STR("Paradox Loader Init!\n"));

    EFI_STATUS Status;
    UINTN HandleCount = 0;
    EFI_HANDLE *HandleBuffer = NULL;
    Status = uefi_call_wrapper((void *)gBS->LocateHandleBuffer, 5, ByProtocol, &FileSystemProtocol, NULL, &HandleCount,
                               &HandleBuffer);
    std::unique_ptr<EFI_HANDLE[]> HandleBufferDeleter(HandleBuffer);

    if (EFI_ERROR(Status)) {
        Print(EFI_STR("Failed to locate File System handles (%d).\n"), Status);
        qemuQuit(1);
        return Status;
    }

    Print(EFI_STR("Found %d file system(s).\n"), HandleCount);

    for (auto i = 0; i < HandleCount; i++) {
        UINT32 VolumeId = 0;
        EFI_STATUS Status = getVolumeId(ImageHandle, HandleBuffer[i], &VolumeId);

        if (!EFI_ERROR(Status)) {
            Print(EFI_STR("Volume Serial Number: %04X-%04X\n"), (VolumeId >> 16) & 0xFFFF, VolumeId & 0xFFFF);
        } else if (Status == EFI_UNSUPPORTED) {
            Print(EFI_STR("Supported file system block, but it is not formatted as FAT.\n"));
        } else {
            Print(EFI_STR("Failed to read raw blocks (Status Code: %r)\n"), Status);
        }
    }

    qemuQuit(0);

    for (;;) {
    }

    return EFI_SUCCESS;
}
