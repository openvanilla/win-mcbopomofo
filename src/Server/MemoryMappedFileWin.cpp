// Windows adaptation of MemoryMappedFile for Win-McBopomofo
#include "MemoryMappedFile.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <utility>
#include <string>

namespace McBopomofo {

MemoryMappedFile::MemoryMappedFile(MemoryMappedFile&& other) noexcept
    : fd_(std::exchange(other.fd_, -1)),
      data_(std::exchange(other.data_, nullptr)),
      length_(std::exchange(other.length_, 0)) {}

MemoryMappedFile& MemoryMappedFile::operator=(
    MemoryMappedFile&& other) noexcept {
  close();
  fd_ = std::exchange(other.fd_, -1);
  data_ = std::exchange(other.data_, nullptr);
  length_ = std::exchange(other.length_, 0);
  return *this;
}

MemoryMappedFile::~MemoryMappedFile() { close(); }

bool MemoryMappedFile::open(const char* path) {
    close();

    // Convert UTF-8 path to UTF-16 for Windows APIs
    int wlen = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);
    if (wlen == 0) return false;
    std::wstring wpath(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, path, -1, &wpath[0], wlen);

    HANDLE hFile = CreateFileW(wpath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(hFile, &fileSize) || fileSize.QuadPart == 0) {
        CloseHandle(hFile);
        return false;
    }

    HANDLE hMapping = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (hMapping == NULL) {
        CloseHandle(hFile);
        return false;
    }

    void* mappedData = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(hMapping);

    if (mappedData == NULL) {
        CloseHandle(hFile);
        return false;
    }
    
    CloseHandle(hFile);
    
    fd_ = 0; // Indicating success
    data_ = mappedData;
    length_ = static_cast<size_t>(fileSize.QuadPart);

    return true;
}

void MemoryMappedFile::close() {
    if (data_ != nullptr) {
        UnmapViewOfFile(data_);
        data_ = nullptr;
    }
    fd_ = -1;
    length_ = 0;
}

}  // namespace McBopomofo
