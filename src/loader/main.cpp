#include <expected>
#include <memory>

#include "efi.hpp"
#include "qemu.h"

template <typename T> using Result = std::expected<T, EFI_STATUS>;

static Result<EFI_GUID> getPartitionUuid(EFI_HANDLE PartitionHandle) {
    EFI_DEVICE_PATH *DevicePath = DevicePathFromHandle(PartitionHandle);
    if (DevicePath == NULL) {
        return std::unexpected(EFI_DEVICE_ERROR);
    }

    EFI_DEVICE_PATH *Node = DevicePath;
    while (!IsDevicePathEnd(Node)) {
        if (DevicePathType(Node) == MEDIA_DEVICE_PATH && DevicePathSubType(Node) == MEDIA_HARDDRIVE_DP) {

            HARDDRIVE_DEVICE_PATH *HdNode = (HARDDRIVE_DEVICE_PATH *)Node;
            if (HdNode->MBRType == SIGNATURE_TYPE_GUID || HdNode->SignatureType == SIGNATURE_TYPE_GUID) {
                return *(EFI_GUID *)HdNode->Signature;
            }

            // Found a hard drive node, but it's not a GPT partition
            return std::unexpected(EFI_UNSUPPORTED);
        }

        Node = NextDevicePathNode(Node);
    }

    return std::unexpected(EFI_NOT_FOUND);
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

    for (UINTN i = 0; i < HandleCount; i++) {
        auto PartUuidResult = getPartitionUuid(HandleBuffer[i]);
        if (!PartUuidResult) {
            continue;
        }
        Print(EFI_STR("Found partition UUID: %g\n"), &PartUuidResult.value());
    }

    qemuQuit(0);

    for (;;) {
    }

    return EFI_SUCCESS;
}
