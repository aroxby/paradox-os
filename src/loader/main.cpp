#include <memory>

#include "config.h"
#include "efi.hpp"
#include "fileio.h"
#include "fixedarray.h"
#include "list.h"
#include "pair.h"
#include "qemu.h"
#include "result.h"
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

static Result<ConfigArgs> parseConfig(String &configContent) {
    ConfigArgs config;
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

static Result<UINT32> intFromHex(const CHAR16 *str, size_t length) {
    UINT32 ret;
    for (size_t i = 0; i < length && str[i]; i++) {
        ret *= 16;
        if (str[i] >= L'0' && str[i] <= L'9') {
            ret += str[i] - L'0';
        } else if (str[i] >= L'a' && str[i] <= L'f') {
            ret += str[i] - L'a' + 10;
        } else if (str[i] >= L'A' && str[i] <= L'F') {
            ret += str[i] - L'A' + 10;
        } else {
            return std::unexpected(EFI_INVALID_PARAMETER);
        }
    }
    return ret;
}

static Result<EFI_GUID> parseUUID(const String &uuidStr) {
    EFI_GUID guid;

    if (uuidStr.length() != 36 || uuidStr[8] != L'-' || uuidStr[13] != L'-' || uuidStr[18] != L'-' ||
        uuidStr[23] != L'-') {
        return std::unexpected(EFI_INVALID_PARAMETER);
    }

    auto parseResult = intFromHex(uuidStr.c_str(), 8);
    if (!parseResult) {
        return std::unexpected(EFI_INVALID_PARAMETER);
    }
    guid.Data1 = parseResult.value();

    parseResult = intFromHex(uuidStr.c_str() + 9, 4);
    if (!parseResult) {
        return std::unexpected(EFI_INVALID_PARAMETER);
    }
    guid.Data2 = parseResult.value();

    parseResult = intFromHex(uuidStr.c_str() + 14, 4);
    if (!parseResult) {
        return std::unexpected(EFI_INVALID_PARAMETER);
    }
    guid.Data3 = parseResult.value();

    for (int i = 0; i < 2; i++) {
        parseResult = intFromHex(uuidStr.c_str() + 19 + i * 2, 2);
        if (!parseResult) {
            return std::unexpected(EFI_INVALID_PARAMETER);
        }
        guid.Data4[i] = parseResult.value();
    }

    for (int i = 2; i < 8; i++) {
        parseResult = intFromHex(uuidStr.c_str() + 20 + i * 2, 2);
        if (!parseResult) {
            return std::unexpected(EFI_INVALID_PARAMETER);
        }
        guid.Data4[i] = parseResult.value();
    }

    return guid;
}

static Result<Config> unpackConfig(const ConfigArgs &args) {
    // `static` is omitted here because it requires additional code not present with -nostdlib
    const String requiredKeys[] = {(CHAR16 *)L"kernelDevice", (CHAR16 *)L"kernelPath"};
    bool haveAllKeys = true;

    for (auto &requiredKey : requiredKeys) {
        auto it = args.find(requiredKey);
        if (it == args.end() || it->second.length() == 0) {
            Print(EFI_STR("Missing required config key: %s\n"), requiredKey.c_str());
            // Continue to print all missing keys
            haveAllKeys = false;
        }
    }

    if (!haveAllKeys) {
        return std::unexpected(EFI_INVALID_PARAMETER);
    }

    auto uuidResult = parseUUID(args.find((CHAR16 *)L"kernelDevice")->second);
    if (!uuidResult) {
        return std::unexpected(EFI_INVALID_PARAMETER);
    }

    Config config = {.kernelPath = args.find((CHAR16 *)L"kernelPath")->second, .partitionUuid = uuidResult.value()};
    return config;
}

static bool operator==(const EFI_GUID &lhs, const EFI_GUID &rhs) {
    const UINT32 *lhsData = reinterpret_cast<const UINT32 *>(&lhs);
    const UINT32 *rhsData = reinterpret_cast<const UINT32 *>(&rhs);
    return lhsData[0] == rhsData[0] && lhsData[1] == rhsData[1] && lhsData[2] == rhsData[2] && lhsData[3] == rhsData[3];
}

static EFI_STATUS runEfi(EFI_HANDLE *imageHandle, EFI_FILE_HANDLE fs, String &path) {
    std::unique_ptr<EFI_DEVICE_PATH> devicePath;
    devicePath.reset(FileDevicePath(fs, path.data()));
    if (devicePath == NULL) {
        return EFI_OUT_OF_RESOURCES;
    }
    Print(EFI_STR("Kernel device path: %s\n"), DevicePathToStr(devicePath.get()));

    EFI_HANDLE secondImageHandle = NULL;
    EFI_STATUS status =
        uefi_call_wrapper((void *)BS->LoadImage, 6, FALSE, *imageHandle, devicePath.get(), NULL, 0, &secondImageHandle);

    if (EFI_ERROR(status)) {
        return status;
    }

    devicePath.reset();

    if (!EFI_ERROR(status)) {
        status = uefi_call_wrapper((void *)BS->StartImage, 3, secondImageHandle, NULL, NULL);
        uefi_call_wrapper((void *)BS->UnloadImage, 1, secondImageHandle);
    }

    return status;
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
    auto &&configArgs = parseConfigResult.value();

    for (auto &value : configArgs) {
        Print(EFI_STR("Config value: [%s] = [%s]\n"), value.first.c_str(), value.second.c_str());
    }

    auto unpackConfigResult = unpackConfig(configArgs);
    if (!unpackConfigResult) {
        Print(EFI_STR("Failed to unpack config (%u)\n"), unpackConfigResult.error());
        quit(1);
        return unpackConfigResult.error();
    }
    auto &&config = unpackConfigResult.value();

    auto fileSystemHandlesResult = getFileSystemHandles();
    if (!fileSystemHandlesResult) {
        Print(EFI_STR("Failed to get file system handles (%u)\n"), fileSystemHandlesResult.error());
        quit(1);
        return fileSystemHandlesResult.error();
    }
    auto &&fileSystemHandles = fileSystemHandlesResult.value();

    EFI_FILE_HANDLE kernelFS = nullptr;
    for (auto &handle : fileSystemHandles) {
        auto partUuidResult = getPartitionUuid(handle);
        if (!partUuidResult) {
            continue;
        }
        Print(EFI_STR("Found FS with partition UUID: %g\n"), &*partUuidResult);
        if (*partUuidResult == config.partitionUuid) {
            kernelFS = handle;
            // break;
        }
    }

    if (kernelFS == nullptr) {
        Print(EFI_STR("Failed to find file system with partition UUID: %g\n"), &config.partitionUuid);
        quit(1);
        return EFI_NOT_FOUND;
    }

    auto status = runEfi(&ImageHandle, kernelFS, config.kernelPath);
    if (EFI_ERROR(status)) {
        Print(EFI_STR("Failed to run kernel (%u)\n"), status);
        quit(1);
        return status;
    }

    quit(0);
    return EFI_SUCCESS;
}
