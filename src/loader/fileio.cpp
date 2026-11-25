#include <memory>

#include "efi.hpp"
#include "result.h"
#include "scopeexit.h"
#include "string.h"

static UINT64 fileSize(EFI_FILE_HANDLE fileHandle) {
    EFI_FILE_INFO *fileInfo;
    UINT64 size;
    fileInfo = LibFileInfo(fileHandle);
    if (!fileInfo) {
        return 0;
    }
    size = fileInfo->FileSize;
    FreePool(fileInfo);
    return size;
}

static std::unique_ptr<CHAR16[]> utf8ToUtf16(const char *utf8Str) {
    size_t length = 0;
    while (utf8Str[length++] != '\0') {
    }

    std::unique_ptr<CHAR16[]> buffer(new CHAR16[length]);
    for (size_t i = 0; i < length; ++i) {
        buffer[i] = utf8Str[i];
    }
    return buffer;
}

Result<String> loadFile(EFI_HANDLE imageHandle, const String &filePath) {
    EFI_STATUS status;
    EFI_LOADED_IMAGE *loadedImage = NULL;

    status = uefi_call_wrapper((void *)BS->HandleProtocol, 3, imageHandle, &LoadedImageProtocol, (VOID **)&loadedImage);
    if (EFI_ERROR(status)) {
        return std::unexpected(EFI_UNSUPPORTED);
    }

    EFI_FILE_HANDLE volumeHandle = LibOpenRoot(loadedImage->DeviceHandle);
    if (volumeHandle == NULL) {
        return std::unexpected(EFI_DEVICE_ERROR);
    }
    ScopeExit volumeClose([volumeHandle]() { uefi_call_wrapper((void *)volumeHandle->Close, 1, volumeHandle); });

    EFI_FILE_HANDLE fileHandle;
    status = uefi_call_wrapper((void *)volumeHandle->Open, 5, volumeHandle, &fileHandle, filePath.c_str(),
                               EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
    if (EFI_ERROR(status)) {
        return std::unexpected(EFI_NOT_FOUND);
    }
    ScopeExit fileClose([fileHandle]() { uefi_call_wrapper((void *)fileHandle->Close, 1, fileHandle); });

    UINTN readSize = fileSize(fileHandle);
    if (readSize == 0) {
        return std::unexpected(EFI_END_OF_FILE);
    }

    std::unique_ptr<UINT8[]> buffer(new UINT8[readSize + 1]);
    status = uefi_call_wrapper((void *)fileHandle->Read, 3, fileHandle, &readSize, buffer.get());
    if (EFI_ERROR(status)) {
        return std::unexpected(status);
    }
    buffer[readSize] = '\0';

    return String(utf8ToUtf16(reinterpret_cast<const char *>(buffer.get())).get());
}
