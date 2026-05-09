# Server 狀態改變時 Client 的對應規則

## 1. 目的

本文檔定義目前系統中：

- Server 內部 `InputState`
- `ServerUI` 轉出的 `IPC::StateUpdatePayload`
- Client `CStateEditSession`

三者之間的對應方式。

## 2. 資料流

當 Server 狀態改變時，實際流程如下：

1. `KeyHandler` 或 `InputController` 產生新的 `InputState`
2. `InputController::ChangeState()` 呼叫 `UIInterface`
3. `ServerUI::Update()` 或 `ServerUI::Reset()` 將 state 投影成 `StateUpdatePayload`
4. payload 經 `NamedPipe` 回到 Client
5. Client 在 `McBopomofoTIP::OnKeyDown()` 儲存 `_lastState`
6. Client 建立 `CStateEditSession`
7. `CStateEditSession::DoEditSession()` 根據 payload 執行：
   - commit
   - 建立或更新 composition
   - 設定 caret
   - 更新 candidate / tooltip window

## 3. Payload 欄位語意

`StateUpdatePayload` 的主要欄位：

- `commitString`
  要直接提交到應用程式的文字。
- `composingBuffer`
  要顯示成 preedit / composition 的文字。
- `cursorIndex`
  composing buffer 內的 UTF-8 cursor offset。
- `candidateIndex`
  candidate list 目前選中的索引。
- `candidates`
  candidate window 顯示的字串列表。
- `tooltip`
  tooltip window 顯示的輔助文字。
- `markStart`, `markEnd`
  preedit 中的標記範圍。
- `forceVertical`
  candidate window 是否強制用垂直布局。

## 4. Server 對各種 state 的映射

實作位於 `src/Server/main.cpp` 的 `ServerUI::Update()`。

### `Inputting`

- `composingBuffer = inputting->composingBuffer`
- `cursorIndex = inputting->cursorIndex`
- `candidates = []`

Client 對應：

- 若 composition 不存在，建立 composition
- 寫入 composing text
- 設定 caret

### `ChoosingCandidate`

- `composingBuffer = choosing->composingBuffer`
- `cursorIndex = choosing->cursorIndex`
- `candidates = candidate values`

Client 對應：

- 維持 composing text
- 顯示 candidate window
- 依 `candidateIndex` 高亮目前項目

### `SelectingDictionary`

- `composingBuffer = previousState->composingBuffer`
- `candidates = menu`
- `forceVertical = true`

Client 對應：

- 保持既有 preedit
- 顯示垂直候選清單

### `ShowingCharInfo`

- `composingBuffer = previousState->composingBuffer`
- `candidates = 兩行資訊字串`
- `forceVertical = true`

### `Marking`

- `composingBuffer = marking->composingBuffer`
- `markStart`, `markEnd` 設定標記範圍

Client 對應：

- 更新 composing text
- 套用 marked display attribute

### `AssociatedPhrases`

- `composingBuffer = assoc->composingBuffer`
- `candidates = 聯想詞列表`

### `AssociatedPhrasesPlain`

- `composingBuffer = ""`
- `cursorIndex = 0`
- `candidates = 聯想詞列表`

Client 對應：

- 不建立 composition
- 只顯示 candidate window

### `NumberInput`

- `composingBuffer = "[數字] ..."`
- `candidates = 轉換結果列表`
- `forceVertical = true`

### `Big5`

- `composingBuffer = big5->composingBuffer()`
- `candidates = []`

Client 對應：

- 顯示 preedit
- 不顯示 candidate window

### `Iroha`

- `composingBuffer = iroha->composingBuffer()`
- `candidates = []`

### `SelectingFeature`

- `composingBuffer = ""`
- `candidates = feature names`
- `forceVertical = true`

Client 對應：

- 不建立 composition
- 只顯示 candidate window

### `SelectingDateMacro`

- `composingBuffer = ""`
- `candidates = macro menu`
- `forceVertical = true`

Client 對應：

- 不建立 composition
- 只顯示 candidate window

這個狀態在選取項目後常會直接進入 commit-only payload。

### `IrohaCandidate`

- `composingBuffer = iroha->composingBuffer()`
- `candidates = iroha candidates`

### `CustomMenu`

- `composingBuffer = customMenu->composingBuffer`
- `candidates = entry names`

## 5. `Reset()` 的語意

`ServerUI::Reset()` 會：

1. 暫存現有 `commitString`
2. 重建一個乾淨的 `StateUpdatePayload`
3. 再把 `commitString` 放回去

這代表：

- reset 會清掉 composing/candidate/tooltip/mark
- 但不會吃掉已經準備提交的文字

這也是為什麼 client 可能收到：

- `commitString != ""`
- `composingBuffer == ""`
- `candidates.empty() == true`

## 6. Client 套用規則

實作位於 `src/Client/StateEditSession.cpp`。

### 規則 A：先看 `commitString`

如果 `commitString` 不為空：

- 若已有 TSF composition，使用 composition range 寫入後結束 composition
- 若沒有 composition，直接 `InsertTextAtSelection`

### 規則 B：再看 `composingBuffer`

如果 `composingBuffer` 不為空：

- 若尚未有 composition，先 `StartComposition`
- 更新 preedit 文字
- 設定 caret
- 套用 display attributes

### 規則 C：若 `composingBuffer` 為空但已有 composition

- 清空 range
- 清掉 display attributes
- `EndComposition`

### 規則 D：candidate / tooltip 依 payload 顯示

- `tooltip` 非空時顯示 tooltip，隱藏 candidate
- 否則更新 candidate window

## 7. commit-only payload 的特殊規則

若 payload 符合：

- `commitString != ""`
- `composingBuffer == ""`
- Client 端 `_pComposition == nullptr`

則視為 direct commit without composition。

這條路徑有兩個重要限制：

1. 先在 `McBopomofoTIP::OnKeyDown()` 進 edit session 前隱藏 auxiliary UI
2. 在 `DoEditSession()` 中 commit 完就直接返回，不再繼續操作 candidate / tooltip / selection 相關 UI

原因是某些 TSF host，尤其是新版 Notepad，對這類 write edit session 很敏感。

## 8. 設計原則

若未來新增新的 server state，應遵守以下順序：

1. 先定義它在邏輯上是不是 `NotEmpty`
2. 再決定 `ServerUI::Update()` 如何投影成 payload
3. 最後確認 Client 應把它當成：
   - composition state
   - candidate-only state
   - commit-only state

不要反過來以「client 想怎麼畫」去扭曲 server state 本身的語意，例如為 candidate-only state 人工塞假的 composing buffer。
