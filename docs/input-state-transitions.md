# Input State Transitions

## 1. Purpose

This document describes the meanings of various `InputState` classes in `src/Server/InputState.h`, as well as their primary transition paths in `KeyHandler` / `InputController`.

This is an implementation-oriented document. It does not attempt to cover every key branch but instead summarizes the main states and common paths.

## 2. State Grouping

### Base States

- `Empty`
  The base state indicating no input content.
- `EmptyIgnoringPrevious`
  Discards the previous state and does not produce additional side effects.
- `Committing`
  Represents the intent to commit text; it is not a stable resting state.
- `StateSequence`
  Triggered by a single key, processes multiple states sequentially.

### States with a Composing Buffer

- `NotEmpty`
  The common base class for all states with a preedit buffer.
- `Inputting`
- `ChoosingCandidate`
- `ChoosingPunctuationList`
- `Marking`
- `SelectingDictionary`
- `ShowingCharInfo`
- `AssociatedPhrases`
- `NumberInput`
- `CustomMenu`

### Candidate-Only or Special States without a Composing Buffer

- `AssociatedPhrasesPlain`
- `Big5`
- `Iroha`
- `IrohaCandidate`
- `SelectingDateMacro`
- `SelectingFeature`

Note:

- Although `Big5` / `Iroha` can display strings, they are not `NotEmpty`.
- `SelectingFeature` / `SelectingDateMacro` are candidate-only states.
- These states should not be treated as general TSF composition states.

## 3. Primary Transition Paths

### State Diagram

```mermaid
stateDiagram-v2
    [*] --> Empty
    
    Empty --> Inputting : Input Bopomofo
    Empty --> SelectingFeature : Trigger Feature Menu
    Empty --> ChoosingPunctuationList : Input Punctuation
    
    Inputting --> ChoosingCandidate : Space / Arrow Keys
    Inputting --> ChoosingPunctuationList : Trigger Punctuation List
    Inputting --> Marking : Shift + Left/Right
    Inputting --> Committing : Confirm Input
    
    ChoosingCandidate --> Committing : Select Candidate
    ChoosingCandidate --> AssociatedPhrases : Trigger Phrases
    ChoosingCandidate --> Inputting : Cancel/Edit
    
    ChoosingPunctuationList --> Committing : Select Punctuation
    ChoosingPunctuationList --> Inputting : Cancel (if from Inputting)
    ChoosingPunctuationList --> EmptyIgnoringPrevious : Cancel (if from Empty)
    
    SelectingFeature --> Big5 : Select Big5
    SelectingFeature --> SelectingDateMacro : Select Date Macro
    SelectingFeature --> NumberInput : Select Number Input
    SelectingFeature --> Iroha : Select Iroha
    
    Big5 --> Committing : Confirm Hex
    Big5 --> Empty : Cancel
    
    SelectingDateMacro --> Committing : Select Date
    SelectingDateMacro --> Empty : Cancel
    
    Iroha --> IrohaCandidate : Continue
    IrohaCandidate --> Committing : Select Candidate
    
    Marking --> SelectingDictionary : Dictionary feature
    Marking --> Inputting : Cancel Marking
    
    SelectingDictionary --> ShowingCharInfo : Select Character Info
    ShowingCharInfo --> SelectingDictionary : Back
    
    AssociatedPhrases --> Committing : Select Phrase
    AssociatedPhrases --> Inputting : Cancel
    
    Committing --> Empty : Auto transition (CommitString)
```

### 3.1 Normal Input

Common path:

1. `Empty`
2. `Inputting`
3. `ChoosingCandidate` or `Committing`
4. `Empty` or return to `Inputting`

Explanation:

- After the user inputs Bopomofo symbols, `KeyHandler::buildInputtingState()` creates an `Inputting` state.
- Under appropriate conditions, the space bar or arrow keys can transition to `ChoosingCandidate`.
- If directly confirmed, it may transition to `Committing`.
- `InputController::ChangeState()` converts `Committing` into `ui_->CommitString(...)`, and then falls back to `Empty`.

### 3.2 Punctuation Candidates

Common path:

1. `Inputting` or `Empty`
2. `ChoosingPunctuationList`
3. `Committing` or `Inputting` / `EmptyIgnoringPrevious`

### 3.3 Feature Menu

Common path:

1. `Empty` or other states
2. `StateSequence(Empty -> SelectingFeature)`
3. `SelectingFeature`
4. Enters depending on the selection:
   - `Big5`
   - `SelectingDateMacro`
   - `NumberInput`
   - `Iroha`

### 3.4 Date Macro

Common path:

1. `SelectingFeature`
2. `SelectingDateMacro`
3. `Committing`
4. `Empty`

This is a very important direct commit path:

- `SelectingDateMacro` does not have a composing buffer.
- Once selected, it directly produces `Committing(text)`.
- The Client may receive `commitString != empty` while `composingBuffer == empty`.

### 3.5 Big5

Common path:

1. `SelectingFeature`
2. `Big5`
3. `Big5` continues to accumulate hexadecimal input.
4. `Committing` or `Empty`

### 3.6 Iroha

Common path:

1. `SelectingFeature`
2. `Iroha`
3. `IrohaCandidate` or `EmptyIgnoringPrevious`
4. `StateSequence(Committing -> Iroha)` or `Empty`

### 3.7 User Phrases, Marking and Character Info

Common path:

1. `Inputting`
2. `Marking`
3. `Inputting` or remains in `Marking`
4. `SelectingDictionary`
5. `ShowingCharInfo`

Explanation:

- `ShowingCharInfo` is a state that displays detailed metadata about a specific character (Unicode code points, encoding info, etc.). 
- It is typically entered from the dictionary selection menu (`SelectingDictionary`) when the user chooses "Character Information".

### 3.8 Associated Phrases

Common path:

1. `Inputting` or `ChoosingCandidate`
2. `AssociatedPhrases` or `AssociatedPhrasesPlain`
3. Returns to `Inputting`, `Committing`, or `Empty` after character selection.

## 4. The Specificity of `Committing`

`Committing` is not a UI state, but an action state.

In `InputController::ChangeState()`:

1. If the new state is `Committing`
2. It will call `ui_->CommitString(text)`
3. It then replaces the state with `Empty`

Therefore:

- The Server logically can produce `Committing`.
- But the client will not receive a stable state named `Committing`.
- What the client receives is:
    - `commitString` is populated
    - Then combined with `Reset()` / `Update()` to form the final payload.

## 5. Difference between `Empty` and `EmptyIgnoringPrevious`

- `Empty`
  Allows the previous state to produce side effects, such as committing.
- `EmptyIgnoringPrevious`
  Explicitly indicates to discard the previous state and not to rely on the previous state anymore.

In `InputController::ChangeState()`, both will eventually cause the controller to fall back to `Empty`, but with different semantics.

## 6. Common Rules for Candidate States

In `InputController`, the following states are treated as candidate states:

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

Common characteristics of these states:

- `HandleKey()` will first branch into `HandleCandidateKey()`.
- `candidateIndex_` is managed by `InputController`.
- Arrow keys, Home/End, PageUp/PageDown, and space bar page turning are all handled at this layer.

## 7. Documentation Maintenance Principles

If a new `InputState` type is added, you must synchronously update at least:

1. The state grouping in this document.
2. The description of `CandidateCount()` / `IsCandidateState()`.
3. How `ServerUI::Update()` maps the payload.
4. How the Client displays or commits it.
