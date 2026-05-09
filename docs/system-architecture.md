# Win-McBopomofo 系統架構

## 1. 架構總覽

目前系統採用 Client/Server 架構，由四個主要部分組成：

1. `src/Server`
   單一背景程序，負責輸入法核心邏輯、狀態管理、設定與詞庫載入。
2. `src/Client`
   TSF TIP DLL，載入到前景應用程式程序內，負責攔截按鍵與操作 TSF composition。
3. `src/Common`
   Client 與 Server 共用的 IPC、序列化與通用工具。
4. `src/ConfigApp`
   獨立設定程式，負責修改 INI 設定並通知 Server reload。

## 2. 元件責任

### Server

核心入口在 `src/Server/main.cpp`。

主要責任：

- 啟動 `NamedPipeServer`
- 建立 `KeyHandler`
- 建立 `InputController`
- 載入並套用 `Settings`
- 接收 Client 傳來的 key event / select candidate / reload / reset 指令
- 將 `InputState` 映射為 `IPC::StateUpdatePayload`

Server 內部又可分成兩層：

- `KeyHandler`
  純輸入法邏輯層，決定輸入、選字、標點、特殊模式與狀態轉換。
- `InputController`
  互動協調層，負責：
  - 決定是否進入 candidate key handling
  - 管理 `candidateIndex_`
  - 處理翻頁、移動、取消、選取候選字
  - 把 `Committing` 轉成 `UIInterface::CommitString()`

### Client

核心入口在 `src/Client/McBopomofoTIP.cpp`。

主要責任：

- 透過 TSF `ITfKeyEventSink` 攔截按鍵
- 將按鍵轉成 IPC request，送給 Server
- 接收 `StateUpdatePayload`
- 建立 `CStateEditSession`
- 在 edit session 內更新：
  - `ITfComposition`
  - composing string
  - caret
  - display attribute
  - candidate window
  - tooltip window

Client 自己不判斷語言模型或選字邏輯；它只根據 Server 回傳 payload 做顯示與提交。

### Common

位於 `src/Common`，主要包含：

- `Ipc.h/.cpp`
  定義 `KeyEventPayload`、`SelectCandidatePayload`、`StateUpdatePayload` 與序列化格式。
- `NamedPipe.h/.cpp`
  封裝 Windows Named Pipe server/client。
- `UTFHelper.cpp`
  UTF-8 / UTF-16 轉換。

### ConfigApp

位於 `src/ConfigApp/main.cpp`。

主要責任：

- 讀寫 `Settings`
- 顯示 Win32 GUI
- 儲存後透過 `IPC::SerializeReloadSettings()` 通知 Server 重載設定

## 3. 主要資料流

### 鍵盤事件流

1. 前景應用程式收到按鍵。
2. TSF 呼叫 Client 的 `OnTestKeyDown()` / `OnKeyDown()`。
3. Client 把按鍵轉成 `IPC::KeyEventPayload`。
4. Payload 經 Named Pipe 傳給 Server。
5. Server 呼叫 `InputController::HandleKey()`。
6. `InputController` 可能再呼叫 `KeyHandler` 或 candidate handling 邏輯。
7. `ServerUI` 把結果轉成 `StateUpdatePayload`。
8. Payload 回傳給 Client。
9. Client 在 `CStateEditSession::DoEditSession()` 套用結果。

### 候選字選擇流

1. 使用者在 candidate mode 中按數字、Enter 或空白翻頁。
2. Server 端 `InputController::HandleCandidateKey()` 更新 `candidateIndex_` 或呼叫 `SelectCandidate()`。
3. 若選定候選字，`InputController` 可能進入：
   - `Committing`
   - 另一個 candidate state
   - `Inputting`
   - `Empty`
4. Client 依 payload 更新 preedit 或直接 commit。

## 4. 狀態與顯示的邊界

系統有一個重要分工：

- `InputState`
  是邏輯狀態，不保證能直接顯示。
- `StateUpdatePayload`
  是顯示狀態，是 Server 為 Windows Client 整理過的 UI 投影。

例如：

- `SelectingFeature` 在邏輯上是 candidate-only state
- 它不是 `NotEmpty`
- 因此不應被硬塞假的 composing buffer

這個邊界很重要，因為 Client 是否建立 composition、是否 direct commit，都是由 payload 內容決定，而不是由 state 類型名稱直接決定。

## 5. 為什麼採用 Client/Server

主要原因：

- 避免每個前景程序都載入完整語言模型
- 把核心狀態集中在單一 server process
- 簡化 TSF DLL，只保留 Windows 介面層
- 讓設定程式與輸入法服務都能共用同一份狀態與 reload 機制

代價是：

- 必須處理 IPC
- 必須定義穩定的 payload 格式
- 需要清楚定義 server state 與 client 行為對應

## 6. 目前的重要限制

目前 Server 只維護單一 `InputController` 實例，而不是以「每個輸入焦點 / 每個應用程式 context」切分 session。這代表系統架構仍然偏向單一互動上下文，而不是完整多 session state 管理。

如果未來要支援更嚴格的多視窗、多程序隔離，這會是第一個需要演進的地方。
