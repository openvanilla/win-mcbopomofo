#include <sddl.h>
#include <windows.h>

#include <iostream>

int main() {
  SECURITY_ATTRIBUTES sa;
  sa.nLength = sizeof(sa);
  sa.bInheritHandle = FALSE;
  BOOL res = ConvertStringSecurityDescriptorToSecurityDescriptorA(
      "D:(A;;GA;;;WD)(A;;GA;;;AC)S:(ML;;NW;;;LW)", SDDL_REVISION_1,
      &sa.lpSecurityDescriptor, NULL);

  if (!res) {
    std::cerr << "SDDL failed: " << GetLastError() << std::endl;
    return 1;
  }

  HANDLE hPipe = CreateNamedPipeA(
      "\\\\.\\pipe\\WinMcBopomofo_IPC_Pipe_Test", PIPE_ACCESS_DUPLEX,
      PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
      PIPE_UNLIMITED_INSTANCES, 4096, 4096, 0, &sa);

  if (hPipe == INVALID_HANDLE_VALUE) {
    std::cerr << "CreateNamedPipeA failed: " << GetLastError() << std::endl;
  } else {
    std::cout << "CreateNamedPipeA succeeded!" << std::endl;
    CloseHandle(hPipe);
  }

  LocalFree(sa.lpSecurityDescriptor);
  return 0;
}