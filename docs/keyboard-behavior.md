# Candidate Mode and Space Bar Behavior

## Purpose

This document explains the actual definition of the space bar behavior in the current Windows version, to avoid confusing two different things:

- Whether the space bar is used to "enter candidate mode"
- Once in candidate mode, whether the space bar is used to "turn the page"

## Conclusion

The current system semantics are as follows:

1. The `ChooseCandidateUsingSpace` setting controls:
   Whether it is allowed to enter candidate mode using the space bar from a normal input state.
2. Once candidate mode has been entered:
   The meaning of the space bar is fixed to "next page in the candidate list".

In other words, this setting is not "whether the space bar selects characters in candidate mode", but rather "whether the space bar switches into candidate mode from a normal input state".

## Space Bar in Normal Input State

Implementation is located in `src/Server/KeyHandler.cpp`.

- If the current state is `NotEmpty`, and:
    - The user presses `Shift+Space`, or
    - `ChooseCandidateUsingSpace == false`
- Then the space bar is treated as a literal space character and inserted into the composing buffer.

Conversely:

- If the current state is `NotEmpty`
- And `ChooseCandidateUsingSpace == true`
- And reading is empty

Then pressing the space bar will enter the candidate choosing state.

## Space Bar in Candidate Mode

Implementation is located in `HandleCandidateKey()` of `src/Server/InputController.cpp`.

When the current state is a candidate state, such as:

- `ChoosingCandidate`
- `SelectingDictionary`
- `ShowingCharInfo`
- `AssociatedPhrases`
- `AssociatedPhrasesPlain`
- `NumberInput`
- `SelectingFeature`
- `SelectingDateMacro`
- `IrohaCandidate`
- `CustomMenu`

Pressing the space bar will execute:

- `MoveCandidatePage(true)`

That is, it turns to the next page, rather than directly selecting the current candidate.

## Comparison with the fcitx5 Version

The fcitx5 version also treats "space bar in normal input state" and "space bar in candidate mode" as logic at different layers:

- In the normal input state, `KeyHandler` decides whether to use the space bar to switch into candidate choosing.
- After entering the candidate panel, the candidate list key processing is handled by the candidate mode logic and framework UI.

The previous issue in the Windows version was not missing the `Space -> page down` logic, but that `UI update` was not immediately called after turning the page, so the screen looked as if the page hadn't turned. This issue has been fixed by adding `ui_->Update(...)` in `InputController::HandleCandidateKey()`.

## Note on Setting Name

Currently, the setting UI displays:

- `使用空白鍵選取候選字` (Use Space to Select Candidates)

But according to the actual program behavior, this text is not precise. A description closer to the implementation would be:

- `使用空白鍵進入候選模式` (Use Space to Enter Candidate Mode)

Or:

- `空白鍵用於候選模式切入` (Space Bar Used for Candidate Mode Switch)

If we want to improve user understanding in the future, it is recommended to prioritize modifying the UI copy, rather than modifying the underlying semantics of this setting.
