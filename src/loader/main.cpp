#include <memory>

#include "efi.hpp"
#include "fixedarray.h"
#include "qemu.h"
#include "result.h"
#include "scopeexit.h"
#include "string.h"

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

static Result<FixedArray<EFI_HANDLE>> getFileSystemHandles() {
    EFI_STATUS Status;
    UINTN HandleCount = 0;
    EFI_HANDLE *HandleBuffer = NULL;
    Status = uefi_call_wrapper((void *)gBS->LocateHandleBuffer, 5, ByProtocol, &FileSystemProtocol, NULL, &HandleCount,
                               &HandleBuffer);
    if (EFI_ERROR(Status)) {
        return std::unexpected(Status);
    }

    return FixedArray<EFI_HANDLE>(HandleBuffer, HandleCount);
}

static Result<String> findCfg(EFI_HANDLE ImageHandle) {
    constexpr const auto *CFG_FILE_NAME = u"paradox.cfg";
    EFI_STATUS Status;
    EFI_LOADED_IMAGE *LoadedImage = NULL;

    Status = uefi_call_wrapper((void *)BS->HandleProtocol, 3, ImageHandle, &LoadedImageProtocol, (VOID **)&LoadedImage);
    if (EFI_ERROR(Status)) {
        return std::unexpected(EFI_UNSUPPORTED);
    }

    CHAR16 *FilePathStr = DevicePathToStr(LoadedImage->FilePath);
    if (FilePathStr == NULL) {
        return std::unexpected(EFI_DEVICE_ERROR);
    }

    String CfgPath(FilePathStr);
    size_t LastSlashPos = CfgPath.find_last_of(L'\\');
    if (LastSlashPos == String::npos) {
        return std::unexpected(EFI_NOT_FOUND);
    }

    CfgPath.replace(LastSlashPos + 1, CfgPath.length() - (LastSlashPos + 1), (const CHAR16 *)CFG_FILE_NAME);
    return CfgPath;
}

static UINT64 fileSize(EFI_FILE_HANDLE FileHandle) {
    EFI_FILE_INFO *FileInfo;
    UINT64 Size;
    FileInfo = LibFileInfo(FileHandle);
    if (!FileInfo) {
        return 0;
    }
    Size = FileInfo->FileSize;
    FreePool(FileInfo);
    return Size;
}

static std::unique_ptr<CHAR16[]> utf8ToUtf16(const char *Utf8Str) {
    size_t Length = 0;
    while (Utf8Str[Length++] != '\0') {
    }

    std::unique_ptr<CHAR16[]> Buffer(new CHAR16[Length]);
    for (size_t i = 0; i < Length; ++i) {
        Buffer[i] = Utf8Str[i];
    }
    return Buffer;
}

static Result<String> loadFile(EFI_HANDLE ImageHandle, const String &FilePath) {
    EFI_STATUS Status;
    EFI_LOADED_IMAGE *LoadedImage = NULL;

    Status = uefi_call_wrapper((void *)BS->HandleProtocol, 3, ImageHandle, &LoadedImageProtocol, (VOID **)&LoadedImage);
    if (EFI_ERROR(Status)) {
        return std::unexpected(EFI_UNSUPPORTED);
    }

    EFI_FILE_HANDLE VolumeHandle = LibOpenRoot(LoadedImage->DeviceHandle);
    if (VolumeHandle == NULL) {
        return std::unexpected(EFI_DEVICE_ERROR);
    }
    ScopeExit VolumeClose([VolumeHandle]() { uefi_call_wrapper((void *)VolumeHandle->Close, 1, VolumeHandle); });

    EFI_FILE_HANDLE FileHandle;
    Status = uefi_call_wrapper((void *)VolumeHandle->Open, 5, VolumeHandle, &FileHandle, FilePath.c_str(),
                               EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
    if (EFI_ERROR(Status)) {
        return std::unexpected(EFI_NOT_FOUND);
    }
    ScopeExit FileClose([FileHandle]() { uefi_call_wrapper((void *)FileHandle->Close, 1, FileHandle); });

    UINTN ReadSize = fileSize(FileHandle);
    if (ReadSize == 0) {
        return std::unexpected(EFI_END_OF_FILE);
    }

    std::unique_ptr<UINT8[]> Buffer(new UINT8[ReadSize + 1]);
    Status = uefi_call_wrapper((void *)FileHandle->Read, 3, FileHandle, &ReadSize, Buffer.get());
    if (EFI_ERROR(Status)) {
        return std::unexpected(Status);
    }
    Buffer[ReadSize] = '\0';

    return String(utf8ToUtf16(reinterpret_cast<const char *>(Buffer.get())).get());
}

[[noreturn]] static void quit(unsigned char Status) {
    qemuQuit(Status);
    for (;;) {
    }
}

extern "C" EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);
    Print(EFI_STR("Paradox Loader Init!\n"));

    auto ConfigResult = findCfg(ImageHandle);
    if (!ConfigResult) {
        Print(EFI_STR("Failed to find config path (%u)\n"), ConfigResult.error());
        quit(1);
        return ConfigResult.error();
    }
    Print(EFI_STR("Found config path: %s\n"), ConfigResult.value().c_str());

    auto FileSystemHandlesResult = getFileSystemHandles();
    if (!FileSystemHandlesResult) {
        Print(EFI_STR("Failed to get file system handles (%u)\n"), FileSystemHandlesResult.error());
        quit(1);
        return FileSystemHandlesResult.error();
    }
    auto &FileSystemHandles = FileSystemHandlesResult.value();
    Print(EFI_STR("Found %d file system(s).\n"), FileSystemHandles.size());

    for (auto &Handle : FileSystemHandles) {
        auto PartUuidResult = getPartitionUuid(Handle);
        if (!PartUuidResult) {
            continue;
        }
        Print(EFI_STR("Found partition UUID: %g\n"), &PartUuidResult.value());
    }

    Print(EFI_STR("Loading file: %s\n"), ConfigResult.value().c_str());

    auto LoadFileResult = loadFile(ImageHandle, ConfigResult.value());
    if (!LoadFileResult) {
        Print(EFI_STR("Failed to load file (%u)\n"), LoadFileResult.error());
        quit(1);
        return LoadFileResult.error();
    }
    auto &ConfigContent = LoadFileResult.value();
    Print(EFI_STR("=== CONFIG ===\n"));
    Print(EFI_STR("%s\n"), ConfigContent.c_str());
    Print(EFI_STR("=== CONFIG ===\n"));

    quit(0);
    return EFI_SUCCESS;
}
