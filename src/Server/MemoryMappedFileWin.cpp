// Copyright (c) 2026 and onwards The McBopomofo Authors.
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

// Windows adaptation of MemoryMappedFile for Win-McBopomofo.
//
// On POSIX systems (Linux and macOS), memory-mapped files are typically implemented using
// standard system calls like open(), mmap(), and munmap(). However, Windows does not natively
// support these POSIX APIs.
//
// Since the McBopomofo core engine relies on memory-mapped dictionary files for highly-efficient,
// low-overhead read access to the language model databases, a custom abstraction is needed on Windows.
// This Windows implementation of MemoryMappedFile bridges that gap by wrapping the native Win32
// memory mapping APIs (CreateFileW, CreateFileMappingW, MapViewOfFile, and UnmapViewOfFile).
//
// By providing this Windows-specific implementation behind a unified MemoryMappedFile interface,
// the core engine's database loading logic remains completely platform-independent.
#include "MemoryMappedFile.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <string>
#include <utility>

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

  // To maintain cross-platform interface compatibility with Linux and macOS (where paths
  // are natively UTF-8 encoded char* strings), the core engine passes paths as UTF-8 narrow
  // strings on all platforms. On Windows, however, native APIs like CreateFileW require UTF-16
  // (wchar_t). Therefore, we explicitly convert the UTF-8 path to UTF-16 here to ensure we can
  // correctly open files containing any unicode characters (such as Chinese or non-ASCII paths)
  // while preserving the engine's clean, cross-platform architecture.
  int wlen = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);
  if (wlen == 0) return false;
  std::wstring wpath(wlen, 0);
  MultiByteToWideChar(CP_UTF8, 0, path, -1, &wpath[0], wlen);

  HANDLE hFile = CreateFileW(wpath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
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

  fd_ = 0;  // Indicating success
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
