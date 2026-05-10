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

#include "NamedPipe.h"

#include <sddl.h>

#include <iostream>

namespace McBopomofo {
namespace IPC {

// --- NamedPipeServer ---

NamedPipeServer::NamedPipeServer(const std::string& pipeName,
                                 MessageCallback callback)
    : pipeName_(pipeName),
      callback_(std::move(callback)),
      hThread_(nullptr),
      running_(false) {}

NamedPipeServer::~NamedPipeServer() { Stop(); }

void NamedPipeServer::Start() {
  if (running_) return;
  running_ = true;
  hThread_ = CreateThread(
      nullptr, 0,
      [](LPVOID param) -> DWORD {
        auto* server = static_cast<NamedPipeServer*>(param);
        server->ServerLoop();
        return 0;
      },
      this, 0, nullptr);
}

void NamedPipeServer::Stop() {
  running_ = false;
  if (hThread_) {
    // Connect a dummy client to unblock ConnectNamedPipe if it's waiting
    NamedPipeClient dummy(pipeName_);
    std::string dummyResponse;
    dummy.Call("", dummyResponse);

    WaitForSingleObject(hThread_, 1000);
    CloseHandle(hThread_);
    hThread_ = nullptr;
  }
}

void NamedPipeServer::ServerLoop() {
  // Create security attributes that grant access to everyone, including
  // AppContainers (UWP) WD = Everyone AC = All Application Packages LW = Low
  // Mandatory Level
  SECURITY_ATTRIBUTES sa;
  sa.nLength = sizeof(sa);
  sa.bInheritHandle = FALSE;
  sa.lpSecurityDescriptor = NULL;

  if (!ConvertStringSecurityDescriptorToSecurityDescriptorA(
          "D:(A;;GA;;;WD)(A;;GA;;;AC)S:(ML;;NW;;;LW)", SDDL_REVISION_1,
          &sa.lpSecurityDescriptor, NULL)) {
    std::cerr << "SDDL conversion failed: " << GetLastError() << std::endl;
    // Fallback to default security
    sa.lpSecurityDescriptor = NULL;
  }

  while (running_) {
    HANDLE hPipe =
        CreateNamedPipeA(pipeName_.c_str(), PIPE_ACCESS_DUPLEX,
                         PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                         PIPE_UNLIMITED_INSTANCES, 4096, 4096, 0,
                         sa.lpSecurityDescriptor ? &sa : NULL);

    if (hPipe == INVALID_HANDLE_VALUE) {
      std::cerr << "CreateNamedPipeA failed: " << GetLastError() << std::endl;
      Sleep(1000);
      continue;
    }

    BOOL connected = ConnectNamedPipe(hPipe, NULL)
                         ? TRUE
                         : (GetLastError() == ERROR_PIPE_CONNECTED);

    if (connected && running_) {
      char buffer[4096];
      DWORD bytesRead;
      if (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
        buffer[bytesRead] = '\0';
        std::string request(buffer, bytesRead);

        std::string response = callback_(request);

        DWORD bytesWritten;
        WriteFile(hPipe, response.c_str(), (DWORD)response.length(),
                  &bytesWritten, NULL);
      }
    }

    FlushFileBuffers(hPipe);
    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);
  }

  if (sa.lpSecurityDescriptor) {
    LocalFree(sa.lpSecurityDescriptor);
  }
}

// --- NamedPipeClient ---

NamedPipeClient::NamedPipeClient(const std::string& pipeName)
    : pipeName_(pipeName) {}

NamedPipeClient::~NamedPipeClient() {}

bool NamedPipeClient::Call(const std::string& request, std::string& response) {
  HANDLE hPipe;

  // Try to connect, waiting up to 1 second
  if (!WaitNamedPipeA(pipeName_.c_str(), 1000)) {
    return false;
  }

  hPipe = CreateFileA(pipeName_.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL,
                      OPEN_EXISTING, 0, NULL);

  if (hPipe == INVALID_HANDLE_VALUE) {
    return false;
  }

  DWORD dwMode = PIPE_READMODE_MESSAGE;
  SetNamedPipeHandleState(hPipe, &dwMode, NULL, NULL);

  DWORD bytesWritten;
  if (!WriteFile(hPipe, request.c_str(), (DWORD)request.length(), &bytesWritten,
                 NULL)) {
    CloseHandle(hPipe);
    return false;
  }

  char buffer[4096];
  DWORD bytesRead;
  bool success = false;
  if (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
    buffer[bytesRead] = '\0';
    response = std::string(buffer, bytesRead);
    success = true;
  }

  CloseHandle(hPipe);
  return success;
}

}  // namespace IPC
}  // namespace McBopomofo
