# Input State 轉換文件

## 1. 目的

本文檔描述 `src/Server/InputState.h` 中各種 `InputState` 的意義，以及它們在 `KeyHandler` / `InputController` 中的主要轉換方向。

這是一份實作導向文件，不嘗試覆蓋每一個按鍵分支，只整理主要狀態與常見路徑。

## 2. 狀態分群

### 基礎狀態

- `Empty`
  無輸入內容的基底狀態。
- `EmptyIgnoringPrevious`
  丟棄前一狀態，不產生額外 side effect。
- `Committing`
  代表要提交文字，不是穩定停留狀態。
- `StateSequence`
  同一按鍵觸發多個狀態依序處理。

### 有 composing buffer 的狀態

- `NotEmpty`
  所有有 preedit 的共同基底。
- `Inputting`
- `ChoosingCandidate`
- `ChoosingPunctuationList`
- `Marking`
- `SelectingDictionary`
- `ShowingCharInfo`
- `AssociatedPhrases`
- `NumberInput`
- `CustomMenu`

### 沒有 composing buffer 的 candidate-only 或特殊狀態

- `AssociatedPhrasesPlain`
- `Big5`
- `Iroha`
- `IrohaCandidate`
- `SelectingDateMacro`
- `SelectingFeature`

注意：

- `Big5` / `Iroha` 雖然可顯示字串，但不是 `NotEmpty`
- `SelectingFeature` / `SelectingDateMacro` 是 candidate-only state
- 這些狀態不應被視為一般 TSF composition state

## 3. 主要轉換路徑

### 3.1 一般輸入

常見路徑：

1. `Empty`
2. `Inputting`
3. `ChoosingCandidate` 或 `Committing`
4. `Empty` 或回到 `Inputting`

說明：

- 使用者輸入注音後，`KeyHandler::buildInputtingState()` 建立 `Inputting`
- 在適當條件下，空白鍵或方向鍵可進入 `ChoosingCandidate`
- 若直接確認，可能進入 `Committing`
- `InputController::ChangeState()` 會把 `Committing` 轉為 `ui_->CommitString(...)`，之後再落到 `Empty`

### 3.2 標點候選

常見路徑：

1. `Inputting` 或 `Empty`
2. `ChoosingPunctuationList`
3. `Committing` 或 `Inputting` / `EmptyIgnoringPrevious`

### 3.3 Feature menu

常見路徑：

1. `Empty` 或其他狀態
2. `StateSequence(Empty -> SelectingFeature)`
3. `SelectingFeature`
4. 依選項進入：
   - `Big5`
   - `SelectingDateMacro`
   - `NumberInput`
   - `Iroha`

### 3.4 Date macro

常見路徑：

1. `SelectingFeature`
2. `SelectingDateMacro`
3. `Committing`
4. `Empty`

這是一條很重要的 direct commit 路徑：

- `SelectingDateMacro` 沒有 composing buffer
- 選定後直接產生 `Committing(text)`
- Client 可能收到 `commitString != empty` 且 `composingBuffer == empty`

### 3.5 Big5

常見路徑：

1. `SelectingFeature`
2. `Big5`
3. `Big5` 持續累積十六進位輸入
4. `Committing` 或 `Empty`

### 3.6 Iroha

常見路徑：

1. `SelectingFeature`
2. `Iroha`
3. `IrohaCandidate` 或 `EmptyIgnoringPrevious`
4. `StateSequence(Committing -> Iroha)` 或 `Empty`

### 3.7 使用者詞與標記

常見路徑：

1. `Inputting`
2. `Marking`
3. `Inputting` 或維持 `Marking`
4. `SelectingDictionary` / `ShowingCharInfo` 等延伸狀態

### 3.8 聯想詞

常見路徑：

1. `Inputting` 或 `ChoosingCandidate`
2. `AssociatedPhrases` 或 `AssociatedPhrasesPlain`
3. 選字後回到 `Inputting`、`Committing` 或 `Empty`

## 4. `Committing` 的特殊性

`Committing` 不是 UI state，而是動作 state。

在 `InputController::ChangeState()` 中：

1. 若新狀態是 `Committing`
2. 會呼叫 `ui_->CommitString(text)`
3. 接著把狀態替換成 `Empty`

因此：

- Server 邏輯上可以產生 `Committing`
- 但 client 端不會收到一個名為 `Committing` 的穩定狀態
- client 收到的是：
  - `commitString` 被填入
  - 然後配合 `Reset()` / `Update()` 形成最終 payload

## 5. `Empty` 與 `EmptyIgnoringPrevious` 的差別

- `Empty`
  允許前一狀態產生 side effect，例如 commit。
- `EmptyIgnoringPrevious`
  明確表示丟棄前一狀態，不要再依賴 previous state。

在 `InputController::ChangeState()` 中，兩者最終都會讓 controller 落回 `Empty`，但語意不同。

## 6. candidate state 的共同規則

在 `InputController` 中，以下狀態都被視為 candidate state：

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

這些狀態的共同特性：

- `HandleKey()` 會先分流到 `HandleCandidateKey()`
- `candidateIndex_` 由 `InputController` 管理
- 方向鍵、Home/End、PageUp/PageDown、空白鍵翻頁都在這層處理

## 7. 文件維護原則

若新增新的 `InputState` 類型，至少要同步更新：

1. 本文件的狀態分群
2. `CandidateCount()` / `IsCandidateState()` 的描述
3. `ServerUI::Update()` 如何映射 payload
4. Client 如何顯示或提交
