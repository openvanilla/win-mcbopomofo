# Win-McBopomofo System Architecture

## 1. Architecture Overview

The current system adopts a Client/Server architecture, consisting of four main components:

1. `src/Server`
   A single background process responsible for the core input method logic, state management, settings, language model loading, and custom candidate/tooltip popup windows.
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
- Monitor configuration and user phrase files on the file system, and reload the
  affected runtime data when those files change.
- Receive key event / select candidate / reload / reset commands from the Client.
- Map `InputState` to `IPC::StateUpdatePayload`.
- Own and render the custom `CandidateWindow` and `TooltipWindow` HWNDs.

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
    - TSF candidate UIElement data
- Probe the focused TSF context for caret / range geometry.
- Include the focused HWND and screen-space anchor rectangle in `KeyEventPayload` when geometry is available.

The Client itself does not judge language models or character selection logic. It applies TSF composition / commit behavior based on the payload returned by the Server, and it routes candidate UI decisions between the TSF UIElement path and the Server-owned custom popup path.

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

## 3. State-Driven Input Model

The input method is state-driven. A key event does not directly mutate the UI or
directly emit text. Instead, the Server interprets each key event against the
current input state and produces the next input state. Output text and UI updates
are derived from that state transition.

Conceptually:

```text
(new state, output text, UI update) = f(old state, key event)
```

This has several consequences:

- The same key can have different meanings in different states. For example,
  `Space`, `Enter`, `Esc`, and arrow keys are interpreted differently in
  `Inputting`, `ChoosingCandidate`, `AssociatedPhrases`, `NumberInput`, or
  `CustomMenu`.
- The UI is a projection of state, not the source of truth. Candidate lists,
  composing text, cursor position, marks, hints, tooltips, and candidate window
  behavior are derived from `InputState` and converted into
  `IPC::StateUpdatePayload`.
- Text commit is also modeled as a state transition. `Committing` is consumed by
  `InputController`, converted into `UIInterface::CommitString()`, and then the
  controller returns to `Empty`.
- Reset and cancellation are explicit state transitions. For example,
  `EmptyIgnoringPrevious` means the previous composing buffer should be
  discarded rather than committed.

The practical reducer pipeline is:

```text
InputController::handleKey(old state, key)
    -> KeyHandler / candidate handling
    -> new InputState
    -> InputController::enterNewState(previous state, new state)
    -> commit/reset/candidateIndex side effects
    -> InputController::buildStateUpdatePayload(current state)
    -> Client TSF composition and UI update
```

This design keeps language-model behavior, candidate selection, and special input
modes in the Server state machine, while the Client remains responsible for
applying the resulting TSF composition and commit operations in the foreground
process.

## 4. Main Data Flows

### Keyboard Event Flow

1. The foreground application receives a key press.
2. TSF calls the Client's `OnTestKeyDown()` / `OnKeyDown()`.
3. The Client converts the key press into an `IPC::KeyEventPayload`, including the focused HWND and screen-space anchor rectangle when available.
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

### File-System Reload Flow

The Server also observes runtime data stored on the file system. When the
configuration file or user phrase data changes, the Server reloads the affected
data and applies it to the active input controller and language model.

This allows changes made outside the immediate key event path, such as saving
settings from `ConfigApp` or updating user phrase files, to become visible to the
running input method without requiring the foreground application or TSF Client
to restart.

## 5. Boundary Between State and Display

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

## 6. Why Adopt Client/Server

Main reasons:

- Avoid having every foreground process load the full language model.
- Centralize core states in a single server process.
- Simplify the TSF DLL, retaining only the Windows interface layer.
- Allow both the configuration program and the input method service to share the same state and reload mechanism.

The cost is:

- Must handle IPC.
- Must define a stable payload format.
- Need to clearly define the mapping between server states and client behaviors.

## 7. Internationalization (i18n)

The project implements a native Windows i18n architecture to support multi-lingual environments (primarily Traditional Chinese and English).

- **Encapsulated Encoding**: The core engine and server logic use UTF-8 (`std::string`). Interactions with Windows APIs use UTF-16 (`std::wstring`). Conversions are strictly encapsulated in `src/Common/UTFHelper.h`.
- **Resource Tables**: UI strings are moved into `.rc` resource files using `STRINGTABLE`. Multiple languages are defined using `LANGUAGE` blocks.
- **Dynamic Loading**: Components load strings at runtime using `LoadStringW` (wrapped in `LoadLocalizedStringW`). This allows the IME UI to automatically switch languages based on the user's Windows display language without requiring separate builds.

## 8. User Interface (UI) Layer

The custom popup UI layer lives in the Server process. `CandidateWindow` and `TooltipWindow` are owned, rendered, moved, and hidden by `McBopomofoServer.exe`. The Client still participates in UI routing because only the TSF TIP DLL can safely inspect the focused TSF context and determine the caret or selection rectangle inside the foreground host process.

- **Rendering Engine**: The Server prefers **Direct2D** and **DirectWrite** for high-quality, hardware-accelerated text rendering, with a GDI compatibility path for hosts where D2D popups are not visible.
- **High DPI Support**: Popup coordinates come from the Client's TSF geometry probing. Popup scaling is computed by the Server-owned popup windows with `GetDpiScaleForWindow()` and `WM_DPICHANGED`.
- **Dark Mode Support**: The system automatically detects the Windows "App Mode" (Light/Dark) by querying the registry (`Personalize\AppsUseLightTheme`). UI colors, brushes, and backgrounds are dynamically adjusted to match the system theme.
- **Layered Stacking**: Auxiliary windows (Tooltip and Candidate) are aware of each other's visibility and height, automatically stacking vertically to avoid overlap.

## 9. Current Major Limitations

Currently, the Server only maintains a single `InputController` instance, rather than splitting sessions "per input focus / per application context". This means the system architecture still leans toward a single interaction context, rather than comprehensive multi-session state management.

If stricter multi-window, multi-process isolation needs to be supported in the future, this would be the first area requiring evolution.
