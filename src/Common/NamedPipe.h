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

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <functional>
#include <string>

namespace McBopomofo {
namespace IPC {

class NamedPipeServer {
 public:
  using MessageCallback = std::function<std::string(const std::string&)>;

  NamedPipeServer(const std::string& pipeName, MessageCallback callback);
  ~NamedPipeServer();

  void Start();
  void Stop();

 private:
  void ServerLoop();

  std::string pipeName_;
  MessageCallback callback_;
  HANDLE hThread_;
  bool running_;
};

class NamedPipeClient {
 public:
  NamedPipeClient(const std::string& pipeName);
  ~NamedPipeClient();

  bool Call(const std::string& request, std::string& response);

 private:
  std::string pipeName_;
};

}  // namespace IPC
}  // namespace McBopomofo
