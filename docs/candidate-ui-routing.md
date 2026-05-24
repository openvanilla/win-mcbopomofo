# Candidate UI Routing

This document explains how Win-McBopomofo decides between:

- the custom popup windows (`CandidateWindow` and `TooltipWindow`)
- the standard TSF UIElement path (`CCandidateListUIElement`)

It also explains why different host applications require different popup
rendering strategies.

## Two Candidate UI Paths

The project has two candidate-display paths:

1. Custom popup path
   `CandidateWindow` is the project's own popup window implementation. It is
   responsible for visuals, DPI handling, and positioning.

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

This renderer decision is separate from TSF's `BeginUIElement()` decision:

- `bShow` decides whether the TIP should draw its own popup at all
- the renderer decides whether that popup is drawn with D2D or GDI

Relevant code:

- [src/Client/CandidateWindow.h](C:/Users/user/Works/win-mcbopomofo/src/Client/CandidateWindow.h)
- [src/Client/TooltipWindow.h](C:/Users/user/Works/win-mcbopomofo/src/Client/TooltipWindow.h)

## When Components Are Created

In `McBopomofoTIP::ActivateEx()`:

- the custom `CandidateWindow` is always created
- if `ITfUIElementMgr` is available, `CCandidateListUIElement` is also created

Relevant code:

- [src/Client/McBopomofoTIP.cpp](C:/Users/user/Works/win-mcbopomofo/src/Client/McBopomofoTIP.cpp)

This means:

- `CandidateWindow` is almost always available
- `CCandidateListUIElement` participates only when the host exposes
  `ITfUIElementMgr`

## High-Level Decision Rule

Candidate visibility first depends on whether `state_.candidates` is empty.

- If there are no candidates, both paths are shut down
- If there are candidates, the TSF UIElement path is updated first, then the
  code decides whether to also show the custom popup

The key decision for the custom candidate popup is the `bShow` value returned
from `ITfUIElementMgr::BeginUIElement()`.

That result is stored in:

- `McBopomofoTIP::showCustomCandidateWindow_`

Relevant code:

- [src/Client/McBopomofoTIP.h](C:/Users/user/Works/win-mcbopomofo/src/Client/McBopomofoTIP.h)

Its meaning is:

- `bShow == TRUE`: the host did not take over candidate display; the TIP should
  show its own candidate window
- `bShow == FALSE`: the host or system will handle TSF candidate UI; the TIP
  should not show its own candidate window

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

The current implementation uses host window class as a runtime heuristic.

- If the context window class is `Windows.UI.Core.CoreWindow`
  - `CandidateWindow` uses `GDI`
  - `TooltipWindow` uses `GDI`
- Otherwise
  - `CandidateWindow` uses `D2D`
  - `TooltipWindow` uses `D2D`

Relevant code:

- [src/Client/CandidateWindow.cpp](C:/Users/user/Works/win-mcbopomofo/src/Client/CandidateWindow.cpp)
- [src/Client/TooltipWindow.cpp](C:/Users/user/Works/win-mcbopomofo/src/Client/TooltipWindow.cpp)

The decision is based on `GetClassNameW(ownerHwnd_)`, not on a process-name
whitelist, because window class is a more direct signal of the host's windowing
model.

## What the GDI Path Must Support

The GDI path is not just a debug probe. It must be functionally usable as a
real fallback renderer.

It therefore includes:

1. Candidate highlight
   `CandidateWindow` draws the selected candidate with a highlight background
   and highlight text color.

2. Emoji fallback
   The GDI path splits text into runs and uses:
   - `Microsoft JhengHei UI` for normal text
   - `Segoe UI Emoji` for emoji runs

3. Tooltip parity
   `TooltipWindow` uses the same host-based renderer selection strategy so the
   candidate popup and tooltip do not diverge on the same host.

Relevant code:

- `CandidateWindow` paint path:
  [src/Client/CandidateWindow.cpp](C:/Users/user/Works/win-mcbopomofo/src/Client/CandidateWindow.cpp)
- `TooltipWindow` paint path:
  [src/Client/TooltipWindow.cpp](C:/Users/user/Works/win-mcbopomofo/src/Client/TooltipWindow.cpp)

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

- hide `CandidateWindow`
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
5. store the returned `bShow` in `showCustomCandidateWindow_`
6. use `showCustomCandidateWindow_` to decide whether to also show the custom
   popup

The important point is:

- `CCandidateListUIElement` is updated first
- whether the custom popup is also shown depends on `bShow`

So `CCandidateListUIElement` is not a fallback for `CandidateWindow`. It is the
first notification to the TSF/host path, and the host then tells the TIP
whether the TIP still needs to draw its own popup.

### 3. Candidates Exist but `ITfUIElementMgr` Is Not Available

If `ITfUIElementMgr` is unavailable, or the candidate UIElement was not
created:

- `showCustomCand` stays at its default `true`
- no TSF UIElement routing occurs
- the custom candidate popup is shown directly

### 4. Showing the Custom `CandidateWindow`

`CandidateWindow::UpdateUI()` is called only when both are true:

- `showCustomCand == true`
- `state_.candidates` is not empty

If `showCustomCand == false`, the TSF UIElement path can still be updated, but
the custom popup will not be shown.

### 5. Direct Commit Without Composition

If a state update is a direct commit and there is no active composition:

- hide `CandidateWindow`
- hide tooltip
- if a TSF UIElement is active, call `EndUIElement()`
- stop further UI handling for that edit session

The purpose of this branch is to prevent stale candidate UI after direct
commit.

## What `CCandidateListUIElement::Show()` Means Here

`CCandidateListUIElement::Show(BOOL fShow)` only updates the UIElement's own
shown state.

It does not directly decide whether the custom popup is shown. The custom popup
decision is based on `BeginUIElement()`'s returned `bShow`, which is stored in
`showCustomCandidateWindow_`.

So these should be understood separately:

- `CCandidateListUIElement::Show()`: shown state of the TSF UIElement itself
- `showCustomCandidateWindow_`: whether the TIP should also show its own
  candidate window

## Decision Table

| Condition | TSF `CCandidateListUIElement` | Custom `CandidateWindow` |
| --- | --- | --- |
| `state_.candidates` is empty | closed / `EndUIElement` | hidden |
| candidates exist, no `ITfUIElementMgr` | unavailable | shown |
| candidates exist, `BeginUIElement()` succeeds and `bShow == TRUE` | updated | shown |
| candidates exist, `BeginUIElement()` succeeds and `bShow == FALSE` | updated | not shown |
| direct commit without composition | closed / `EndUIElement` | hidden |

If the custom popup is shown, there is a second-stage renderer decision:

| Custom popup host | Renderer |
| --- | --- |
| `Windows.UI.Core.CoreWindow` | `GDI` |
| other typical desktop hosts | `D2D` |

## One-Sentence Summary

The actual rule is:

First publish candidate data through the TSF UIElement path. If the host tells
the TIP not to draw its own popup, stop there. Otherwise show the project's own
candidate popup, and choose D2D or GDI based on the host's windowing model.
