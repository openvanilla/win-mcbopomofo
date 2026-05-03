#include <windows.h>
#include <iostream>
#include <string>

int main() {
    HANDLE hPipe;
    std::cout << "Attempting to connect to pipe..." << std::endl;
    
    if (!WaitNamedPipeA("\\\\.\\pipe\\WinMcBopomofo_IPC_Pipe", 2000)) {
        std::cerr << "WaitNamedPipe failed: " << GetLastError() << std::endl;
        return 1;
    }

    hPipe = CreateFileA(
        "\\\\.\\pipe\\WinMcBopomofo_IPC_Pipe",
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (hPipe == INVALID_HANDLE_VALUE) {
        std::cerr << "CreateFileA failed: " << GetLastError() << std::endl;
        return 1;
    }

    std::cout << "Successfully connected to pipe!" << std::endl;
    CloseHandle(hPipe);
    return 0;
}