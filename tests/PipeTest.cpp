#include <gtest/gtest.h>
#include "NamedPipe.h"
#include <thread>
#include <chrono>
#include <string>
#include <windows.h>

using namespace McBopomofo::IPC;

TEST(PipeTest, BasicCommunication) {
    // Convert string for CreateNamedPipeA
    std::string pipeNameA = "\\\\.\\pipe\\McBopomofo_Test_Pipe_" + std::to_string(GetCurrentProcessId());

    NamedPipeServer server(pipeNameA, [](const std::string& request) {
        if (request == "hello") return "world";
        return "unknown";
    });

    server.Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        NamedPipeClient client(pipeNameA);
        std::string response;
        bool success = client.Call("hello", response);

        EXPECT_TRUE(success);
        EXPECT_EQ(response, "world");
    }

    // Small delay to ensure the server loop has time to reset the pipe connection
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    {
        NamedPipeClient client2(pipeNameA);
        std::string response2;
        bool success = client2.Call("test", response2);
        EXPECT_TRUE(success);
        EXPECT_EQ(response2, "unknown");
    }

    server.Stop();
}

TEST(PipeTest, ClientFailureOnMissingServer) {
    std::string pipeNameA = "\\\\.\\pipe\\McBopomofo_NonExistent_Pipe_" + std::to_string(GetCurrentProcessId());
    NamedPipeClient client(pipeNameA);
    std::string response;
    // Should fail quickly because of WaitNamedPipe timeout
    bool success = client.Call("hello", response);
    EXPECT_FALSE(success);
}

TEST(PipeTest, ClientFailureOnServerDisconnect) {
    std::string pipeNameA = "\\\\.\\pipe\\McBopomofo_Disconnect_Pipe_" + std::to_string(GetCurrentProcessId());
    
    // We'll use a raw handle to have more control
    HANDLE hPipe = CreateNamedPipeA(
        pipeNameA.c_str(),
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1, 4096, 4096, 0, NULL);
    
    ASSERT_NE(hPipe, INVALID_HANDLE_VALUE);

    std::thread serverThread([hPipe]() {
        if (ConnectNamedPipe(hPipe, NULL) || GetLastError() == ERROR_PIPE_CONNECTED) {
            // Read the request but don't write anything, just disconnect
            char buffer[100];
            DWORD bytesRead;
            ReadFile(hPipe, buffer, sizeof(buffer), &bytesRead, NULL);
            DisconnectNamedPipe(hPipe);
        }
        CloseHandle(hPipe);
    });

    NamedPipeClient client(pipeNameA);
    std::string response;
    bool success = client.Call("hello", response);
    
    EXPECT_FALSE(success);

    serverThread.join();
}
