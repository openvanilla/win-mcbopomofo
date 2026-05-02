#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <functional>

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

} // namespace IPC
} // namespace McBopomofo
