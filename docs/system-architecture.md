# Win-McBopomofo System Architecture

## 1. Architecture Overview

The current system adopts a Client/Server architecture, consisting of four main components:

1. `src/Server`
   A single background process responsible for the core input method logic, state management, settings, and language model loading.
2. `src/Client`
   The TSF TIP DLL, loaded into the foreground application process, responsible for intercepting key presses and operating the TSF composition.
3. `src/Common`
   IPC, serialization, and utility components shared between the Client and Server.
4. `src/ConfigApp`
   A standalone configuration program, responsible for modifying INI settings and notifying the Server to reload.

## 2. Component Responsibilities

### Server

The core entry point is in `src/Server/main.cpp`.

Main responsibilities:

- Start `NamedPipeServer`.
- Create `KeyHandler`.
- Create `InputController`.
- Load and apply `Settings`.
- Receive key event / select candidate / reload / reset commands from the Client.
- Map `InputState` to `IPC::StateUpdatePayload`.

The Server itself can be divided into two layers:

- `KeyHandler`
  The pure input method logic layer, determining input, character selection, punctuation, special modes, and state transitions.
- `InputController`
  The interaction coordination layer, responsible for:
    - Determining whether to enter candidate key handling.
    - Managing `candidateIndex_`.
    - Handling page turning, moving, canceling, and selecting candidates.
    - Converting `Committing` into `UIInterface::CommitString()`.

### Client

The core entry point is in `src/Client/McBopomofoTIP.cpp`.

Main responsibilities:

- Intercept keystrokes via TSF `ITfKeyEventSink`.
- Convert keystrokes into IPC requests and send them to the Server.
- Receive `StateUpdatePayload`.
- Create `CStateEditSession`.
- Update the following within the edit session:
    - `ITfComposition`
    - composing string
    - caret
    - display attribute
    - candidate window
    - tooltip window

The Client itself does not judge language models or character selection logic; it only performs display and commits based on the payload returned by the Server.

### Common

Located in `src/Common`, it primarily contains:

- `Ipc.h/.cpp`
  Defines `KeyEventPayload`, `SelectCandidatePayload`, `StateUpdatePayload`, and serialization formats.
- `NamedPipe.h/.cpp`
  Encapsulates the Windows Named Pipe server/client.
- `UTFHelper.cpp`
  UTF-8 / UTF-16 conversions.

### ConfigApp

Located in `src/ConfigApp/main.cpp`.

Main responsibilities:

- Read and write `Settings`.
- Display the Win32 GUI.
- Notify the Server to reload settings via `IPC::SerializeReloadSettings()` after saving.

## 3. Main Data Flows

### Keyboard Event Flow

1. The foreground application receives a key press.
2. TSF calls the Client's `OnTestKeyDown()` / `OnKeyDown()`.
3. The Client converts the key press into an `IPC::KeyEventPayload`.
4. The payload is sent to the Server via Named Pipe.
5. The Server calls `InputController::HandleKey()`.
6. `InputController` may further call `KeyHandler` or candidate handling logic.
7. `ServerUI` converts the result into a `StateUpdatePayload`.
8. The payload is returned to the Client.
9. The Client applies the result in `CStateEditSession::DoEditSession()`.

### Candidate Selection Flow

1. The user presses a number, Enter, or the space bar to turn the page in candidate mode.
2. The Server's `InputController::HandleCandidateKey()` updates `candidateIndex_` or calls `SelectCandidate()`.
3. If a candidate is selected, `InputController` may enter:
   - `Committing`
   - Another candidate state
   - `Inputting`
   - `Empty`
4. The Client updates the preedit or commits directly based on the payload.

## 4. Boundary Between State and Display

The system has an important division of labor:

- `InputState`
  Is a logical state and does not guarantee it can be displayed directly.
- `StateUpdatePayload`
  Is a display state; it is a UI projection prepared by the Server for the Windows Client.

For example:

- `SelectingFeature` is logically a candidate-only state.
- It is not `NotEmpty`.
- Therefore, a fake composing buffer should not be forcibly inserted into it.

This boundary is important because whether the Client creates a composition or performs a direct commit is determined by the payload contents, not directly by the state type name.

## 5. Why Adopt Client/Server

Main reasons:

- Avoid having every foreground process load the full language model.
- Centralize core states in a single server process.
- Simplify the TSF DLL, retaining only the Windows interface layer.
- Allow both the configuration program and the input method service to share the same state and reload mechanism.

The cost is:

- Must handle IPC.
- Must define a stable payload format.
- Need to clearly define the mapping between server states and client behaviors.

## 6. Current Major Limitations

Currently, the Server only maintains a single `InputController` instance, rather than splitting sessions "per input focus / per application context". This means the system architecture still leans toward a single interaction context, rather than comprehensive multi-session state management.

If stricter multi-window, multi-process isolation needs to be supported in the future, this would be the first area requiring evolution.
