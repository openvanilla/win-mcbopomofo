# Win-McBopomofo Documentation Index

This directory currently contains the following files:

- `development-requirements.md`
  Describes the development environment requirements and setup.
- `keyboard-behavior.md`
  Describes the actual behavior of the space bar, candidate mode, and page flipping.
- `system-architecture.md`
  Describes the overall system architecture and module responsibilities of the Windows version.
- `input-state-transitions.md`
  Describes the `InputState` types and major state transition paths.
- `logging.md`
  Describes the logging mechanism, log file locations, and how to use the logging macros.
- `installer.md`
  Describes what the MSI installer does during installation and provides instructions on how developers can build it.
- `server-client-state-mapping.md`
  Describes how the Client should map and apply updates after Server state changes.
- `IPC_PROTOCOL.md`
  Describes the IPC protocol between the Client (TSF TIP) and the Server.

This documentation describes the actual behavior in the current code, not idealized specifications. If the code changes later, the documentation should be updated accordingly.
