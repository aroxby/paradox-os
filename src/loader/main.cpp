#include <memory>

#include "config.h"
#include "efi.hpp"
#include "fixedarray.h"
#include "list.h"
#include "pair.h"
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

static Result<FixedArray<EFI_FILE_HANDLE>> getFileSystemHandles() {
    EFI_STATUS status;
    UINTN handleCount = 0;
    EFI_FILE_HANDLE *handleBuffer = NULL;
    status = uefi_call_wrapper((void *)gBS->LocateHandleBuffer, 5, ByProtocol, &FileSystemProtocol, NULL, &handleCount,
                               &handleBuffer);
    if (EFI_ERROR(status)) {
        return std::unexpected(status);
    }

    return FixedArray<EFI_FILE_HANDLE>(handleBuffer, handleCount);
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

static Result<Config> parseConfig(String &configContent) {
    Config config;
    auto *configDataStart = configContent.data();
    auto *configDataEnd = configDataStart + configContent.length();
    auto *configLineStart = configDataStart;
    while (configLineStart < configDataEnd) {
        auto *configLineEnd = configLineStart;
        while (configLineEnd < configDataEnd && *configLineEnd != L'\n') {
            ++configLineEnd;
        }
        *configLineEnd = L'\0';

        auto *split = wcschr(configLineStart, (CHAR16)L'=');
        if (!split) {
            Print(EFI_STR("Invalid config line: [%s]\n"), configLineStart);
            return std::unexpected(EFI_INVALID_PARAMETER);
        }
        *split = L'\0';

        config.insert(std::move(Pair<const String, String>{wcsstrip(configLineStart), wcsstrip(split + 1)}));

        configLineStart = configLineEnd;
        do {
            ++configLineStart;
        } while (configLineStart < configDataEnd && *configLineStart == L'\n');
    }
    return config;
}

static bool validateConfig(const Config &config) {
    // `static` is omitted here because it requires additional code not present with -nostdlib
    const String requiredKeys[] = {(CHAR16 *)L"kernelDevice", (CHAR16 *)L"kernelPath"};
    bool valid = true;

    for (auto &requiredKey : requiredKeys) {
        auto it = config.find(requiredKey);
        if (it == config.end() || it->second.length() == 0) {
            Print(EFI_STR("Missing required config key: %s\n"), requiredKey.c_str());
            // Continue to print all missing keys
            valid = false;
        }
    }
    return valid;
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

    auto LoadFileResult = loadFile(ImageHandle, ConfigResult.value());
    if (!LoadFileResult) {
        Print(EFI_STR("Failed to load file (%u)\n"), LoadFileResult.error());
        quit(1);
        return LoadFileResult.error();
    }
    auto &&ConfigContent = LoadFileResult.value();

    auto parseConfigResult = parseConfig(ConfigContent);
    if (!parseConfigResult) {
        Print(EFI_STR("Failed to parse config (%u)\n"), parseConfigResult.error());
        quit(1);
        return parseConfigResult.error();
    }
    auto &&config = parseConfigResult.value();

    for (auto &value : config) {
        Print(EFI_STR("Config value: [%s] = [%s]\n"), value.first.c_str(), value.second.c_str());
    }

    if (!validateConfig(config)) {
        quit(1);
        return EFI_INVALID_PARAMETER;
    }

    auto fileSystemHandlesResult = getFileSystemHandles();
    if (!fileSystemHandlesResult) {
        Print(EFI_STR("Failed to get file system handles (%u)\n"), fileSystemHandlesResult.error());
        quit(1);
        return fileSystemHandlesResult.error();
    }
    auto &&fileSystemHandles = fileSystemHandlesResult.value();

    Map<EFI_GUID, EFI_FILE_HANDLE> partitionMap;
    for (auto &handle : fileSystemHandles) {
        auto partUuidResult = getPartitionUuid(handle);
        if (!partUuidResult) {
            continue;
        }
        Print(EFI_STR("Found FS with partition UUID: %g\n"), &*partUuidResult);
        partitionMap.insert(std::move(Pair<const EFI_GUID, EFI_FILE_HANDLE>{*partUuidResult, handle}));
    }

    for (auto &value : partitionMap) {
        Print(EFI_STR("Partition UUID: %g = %p\n"), &value.first, value.second);
    }

    quit(0);
    return EFI_SUCCESS;
}
