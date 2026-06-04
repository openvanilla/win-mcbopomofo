# Candidate UI Routing

This document explains how Win-McBopomofo divides candidate UI work between
the TSF client DLL and the background server, and how it decides between:

- the custom popup windows (`CandidateWindow` and `TooltipWindow`)
- the standard TSF UIElement path (`CCandidateListUIElement`)

It also explains why the client still participates in candidate UI even though
the custom popup HWNDs now live in the server process.

## Server and Client Responsibilities

Win-McBopomofo has two runtime components:

1. Client: `McBopomofoTIP_v2.dll`
   The client is loaded into the foreground TSF host process. It handles TSF
   activation, key sinks, compositions, display attributes, language-bar items,
   and TSF UIElement registration. It also probes the focused TSF context for
   caret/range geometry and sends that screen-space anchor to the server.

2. Server: `McBopomofoServer.exe`
   The server owns the input engine, language model, settings, user phrase
   files, candidate state, background tray icon, and the custom popup HWNDs.
   `CandidateWindow` and `TooltipWindow` are created by `ServerPopupController`
   in `src/Server/main.cpp`, not by the client DLL.

The boundary is intentional:

- engine state stays in one long-lived server process
- TSF document access stays inside the host process where the client DLL runs
- popup HWND lifetime and rendering are centralized in the server
- the two sides communicate through the named pipe protocol in
  `src/Common/Ipc.h`

The client never directly paints the server popup windows. It only decides
whether TSF composition and UIElement data should be updated, computes where
server popups can be anchored while handling key events, and sends that
geometry as part of `IPC::KeyEventPayload` with:

- the owning/focused HWND value, if available
- the screen-space anchor rectangle

The server decides popup visibility from the latest `StateUpdatePayload`: a
non-empty `candidates` list can show `CandidateWindow`, and a non-empty
`tooltip` can show `TooltipWindow`.

## Two Candidate UI Paths

The project has two candidate-display paths:

1. Custom popup path
   `CandidateWindow` and `TooltipWindow` are the project's own popup window
   implementations. They are created, rendered, moved, and hidden in
   `McBopomofoServer.exe`.

2. TSF UIElement path
   `CCandidateListUIElement` implements the standard TSF candidate list
   interfaces so the host application or the system can consume candidate data
   through TSF.

These paths are not selected once at startup. The decision is made dynamically
on each state update inside `CStateEditSession::DoEditSession()`.

## The Custom Popup Path Also Has Two Renderers

Even after the code decides to use the custom popup windows, not every host can
reliably display the same kind of HWND with the same renderer.

The custom popup path therefore has two internal renderers:

1. `D2D`
   Uses Direct2D and DirectWrite. This is the preferred renderer for normal
   desktop hosts.

2. `GDI`
   Uses traditional GDI drawing. This renderer exists as a compatibility path
   for hosts where the popup HWND is created and positioned correctly but a D2D
   popup still does not become visible.

This renderer decision is separate from the TSF UIElement path:

- the TSF UIElement path publishes candidate data to the host
- the renderer decides whether the server-owned popup is drawn with D2D or GDI

Relevant code:

- [src/Server/CandidateWindow.h](C:/Users/user/Works/win-mcbopomofo/src/Server/CandidateWindow.h)
- [src/Server/TooltipWindow.h](C:/Users/user/Works/win-mcbopomofo/src/Server/TooltipWindow.h)

## When Components Are Created

In `McBopomofoTIP::ActivateEx()`:

- language-bar items are registered
- TSF key/focus/compartment sinks are advised
- if `ITfUIElementMgr` is available, the client creates
  `CCandidateListUIElement` and `CReadingInformationUIElement`

Relevant code:

- [src/Client/McBopomofoTIP.cpp](C:/Users/user/Works/win-mcbopomofo/src/Client/McBopomofoTIP.cpp)

In `McBopomofoServer.exe` startup:

- `ServerPopupController` creates `CandidateWindow` and `TooltipWindow`
- the named pipe server starts accepting key, selection, settings, log, and
  layout requests
- the tray icon and popup message window are owned by the server process

Relevant code:

- [src/Server/main.cpp](C:/Users/user/Works/win-mcbopomofo/src/Server/main.cpp)

This means:

- the custom popup windows are available as long as the server is running
- `CCandidateListUIElement` participates only when the TSF host exposes
  `ITfUIElementMgr`
- the client sends popup anchor geometry to the server as part of key events
  whenever geometry is available

## High-Level Decision Rule

Candidate visibility first depends on whether `state_.candidates` is empty.

- If there are no candidates, both paths are shut down
- If there are candidates, the Client updates the TSF UIElement path when
  available, and the Server-owned custom popup path can show the same candidate
  data when the server has a valid anchor rectangle

## Layout Flow

The server can render popups, but it cannot safely inspect the TSF document
inside the foreground host. The client therefore owns geometry discovery.

For key-driven updates, `McBopomofoTIP::OnKeyDown()` tries to capture the
current TSF selection rectangle while it is already handling the key. It sends
that geometry with `IPC::KeyEventPayload` when available.

For key events, the client uses this fallback order:

1. current composition or cursor range via `ITfContextView::GetTextExt()`
2. current selection via `GetSelection()` and `GetTextExt()`
3. Win32 caret fallback through `GetGUIThreadInfo()`

The chosen rectangle is serialized in `IPC::KeyEventPayload`. The server
receives that request in `src/Server/main.cpp`, stores it in
`ServerPopupController`, and posts a window message so the popup move/show work
happens on the server UI thread.

The server then:

1. combines the latest engine state with the latest key-event anchor geometry
2. updates `CandidateWindow` and/or `TooltipWindow`
3. positions them around the anchor rectangle while keeping them inside the
   monitor work area

## Why Different Apps Need Different Popup Rendering

This is based on observed runtime behavior, not just theory.

On some traditional desktop hosts:

- `BeginUIElement()` returns `bShow == TRUE`
- the custom popup is visible when rendered with D2D

On `Notepads` and similar `Windows.UI.Core.CoreWindow` hosts, logging showed:

- `BeginUIElement()` returns `bShow == TRUE`
- the candidate popup is created successfully
- the popup receives `UpdateUI`
- layout is computed
- the popup is moved to a reasonable screen position
- the popup is not immediately hidden while the candidate state is active
- but a D2D popup still does not become visible

To verify that the problem was not TSF state, fallback logic, or positioning, a
minimal GDI probe was added. With the same owner HWND, same candidate data, and
same coordinates, the popup became visible when drawn with GDI.

That leads to the current engineering conclusion:

- the problem is not the candidate state machine
- the problem is not `BeginUIElement()` or `bShow`
- the problem is not popup creation or positioning
- the problem is host-specific visibility behavior for certain popup/rendering
  combinations

Because of that, a single popup renderer is not reliable enough for every host.

## Current Renderer Selection Strategy

The current server-side popup path prefers D2D/DirectWrite. The GDI renderer is
kept as a compatibility implementation and fallback path for cases where a
D2D render target cannot be created or must be recreated.

Relevant code:

- [src/Server/CandidateWindow.cpp](C:/Users/user/Works/win-mcbopomofo/src/Server/CandidateWindow.cpp)
- [src/Server/TooltipWindow.cpp](C:/Users/user/Works/win-mcbopomofo/src/Server/TooltipWindow.cpp)

## What the GDI Path Must Support

The GDI path is not just a debug probe. It must be functionally usable as a
real fallback renderer.

It therefore includes:

1. Candidate highlight
   `CandidateWindow` draws the selected candidate with a highlight background
   and highlight text color. In vertical layouts, the highlight extends across
   the full content row instead of stopping at the selected glyph width.

2. Emoji fallback
   The GDI path splits text into runs and uses:
   - `Microsoft JhengHei UI` for normal text
   - `Segoe UI Emoji` for emoji runs

3. Keycap styling
   Candidate key labels are rendered with a slightly smaller UI font than the
   candidate text so the fallback renderer stays visually close to the original
   D2D layout.

4. Layout parity
   GDI sizing uses the same run-splitting rules as GDI painting. This avoids
   clipping caused by measuring text with one renderer and drawing it with
   another.

5. Tooltip parity
   `TooltipWindow` keeps the same D2D/GDI rendering capabilities as
   `CandidateWindow` so the candidate popup and tooltip can be maintained
   together.

Relevant code:

- `CandidateWindow` paint path:
  [src/Server/CandidateWindow.cpp](C:/Users/user/Works/win-mcbopomofo/src/Server/CandidateWindow.cpp)
- `TooltipWindow` paint path:
  [src/Server/TooltipWindow.cpp](C:/Users/user/Works/win-mcbopomofo/src/Server/TooltipWindow.cpp)

## Actual Branch Flow

`CStateEditSession::DoEditSession()` has two entry paths that can route into
candidate UI handling:

1. There is an active composing buffer
2. There is no composition, but there are still candidates or tooltip text

The logic is almost identical in both branches.

Relevant code:

- [src/Client/StateEditSession.cpp](C:/Users/user/Works/win-mcbopomofo/src/Client/StateEditSession.cpp)

### 1. No Candidates

If `state_.candidates.empty()`:

- ask the server to hide `CandidateWindow`
- if a TSF UIElement was active, call `EndUIElement()`
- mark `CCandidateListUIElement` as not shown

### 2. Candidates Exist and `ITfUIElementMgr` Is Available

If both are true:

- `pTIP_->GetUIElementMgr() != nullptr`
- `pTIP_->GetCandidateUIElement() != nullptr`

then the TSF UIElement path is updated first.

The flow is:

1. call `CCandidateListUIElement::SetActiveContext()`
2. call `CCandidateListUIElement::UpdateData()`
3. if this is the first show, call `BeginUIElement()`
4. otherwise call `UpdateUIElement()`

The important point is:

- `CCandidateListUIElement` is updated first
- the server-owned custom popup is controlled by server state plus the latest
  key-event anchor geometry, not by a separate client layout request

So `CCandidateListUIElement` is not a fallback for `CandidateWindow`. It is the
notification to the TSF/host path; the custom popup path is maintained by the
server process.

### 3. Candidates Exist but `ITfUIElementMgr` Is Not Available

If `ITfUIElementMgr` is unavailable, or the candidate UIElement was not
created:

- no TSF UIElement routing occurs
- the server can still show the custom candidate popup when it has candidates
  and a valid anchor rectangle

### 4. Showing the Custom `CandidateWindow`

The server calls `CandidateWindow::UpdateUI()` only when both are true:

- `state_.candidates` is not empty
- `ServerPopupController` has valid layout information from a key event

The custom popup visibility is not encoded as a separate IPC command. It is
derived from the latest server state and the latest layout anchor.

### 5. Direct Commit Without Composition

If a state update is a direct commit and there is no active composition:

- ask the server to hide `CandidateWindow`
- hide tooltip
- if a TSF UIElement is active, call `EndUIElement()`
- stop further UI handling for that edit session

The purpose of this branch is to prevent stale candidate UI after direct
commit.

## What `CCandidateListUIElement::Show()` Means Here

`CCandidateListUIElement::Show(BOOL fShow)` only updates the UIElement's own
shown state.

It does not directly decide whether the custom popup is shown. The custom popup
decision is owned by `ServerPopupController` in the server process.

So these should be understood separately:

- `CCandidateListUIElement::Show()`: shown state of the TSF UIElement itself
- `ServerPopupController`: whether the server-owned candidate and tooltip
  windows should be shown from the current state and latest anchor

## Decision Table

| Condition | TSF `CCandidateListUIElement` | Custom `CandidateWindow` |
| --- | --- | --- |
| `state_.candidates` is empty | closed / `EndUIElement` | hidden |
| candidates exist, no `ITfUIElementMgr` | unavailable | shown if the server has an anchor |
| candidates exist, `ITfUIElementMgr` is available | updated | shown if the server has an anchor |
| direct commit without composition | closed / `EndUIElement` | hidden |

## One-Sentence Summary

The actual rule is:

First publish candidate data through the TSF UIElement path from the client.
The client also sends key-event anchor geometry when available, and the server
uses that geometry plus its latest state to render and position the project's
own candidate and tooltip popups.
