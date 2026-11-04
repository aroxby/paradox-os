#include "efi.hpp"
#include "qemu.h"

#include <cstring>
#include <memory>

/**
 * Retrieves the GPT Partition UUID (Signature) from a given partition handle.
 *
 * @param PartitionHandle  The handle to the partition.
 * @param OutGuid          Pointer to an EFI_GUID buffer where the UUID will be copied.
 * @return EFI_SUCCESS if successful, or an appropriate error code.
 */
static EFI_STATUS getPartitionUuid(EFI_HANDLE PartitionHandle, EFI_GUID *OutGuid) {
    if (PartitionHandle == NULL || OutGuid == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    // 1. Fetch the Device Path protocol instance from the handle
    EFI_DEVICE_PATH *DevicePath = DevicePathFromHandle(PartitionHandle);
    if (DevicePath == NULL) {
        return EFI_NOT_FOUND;
    }

    // 2. Walk the device path nodes looking for the Hard Drive Media Node
    EFI_DEVICE_PATH *Node = DevicePath;
    while (!IsDevicePathEnd(Node)) {

        // Type 4 = Media Device Path, SubType 1 = Hard Drive
        if (DevicePathType(Node) == MEDIA_DEVICE_PATH && DevicePathSubType(Node) == MEDIA_HARDDRIVE_DP) {

            HARDDRIVE_DEVICE_PATH *HdNode = (HARDDRIVE_DEVICE_PATH *)Node;

            // PartitionFormat 2 = GPT (1 = MBR)
            // SignatureType 2 = GUID (1 = MBR 4-byte signature)
            if (HdNode->MBRType == SIGNATURE_TYPE_GUID || HdNode->SignatureType == SIGNATURE_TYPE_GUID) {
                // Copy the 16-byte unique partition GUID out
                memcpy(OutGuid, HdNode->Signature, sizeof(EFI_GUID));
                return EFI_SUCCESS;
            }

            // Found a hard drive node, but it's not a GPT partition
            return EFI_UNSUPPORTED;
        }

        Node = NextDevicePathNode(Node);
    }

    return EFI_NOT_FOUND;
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
        EFI_GUID PartUuid;
        Status = getPartitionUuid(HandleBuffer[i], &PartUuid);
        if (EFI_ERROR(Status)) {
            continue;
        }
        Print(EFI_STR("Found partition UUID: %g\n"), &PartUuid);
    }

    qemuQuit(0);

    for (;;) {
    }

    return EFI_SUCCESS;
}
