# Win-McBopomofo IPC Protocol 文件

## 概述

Win-McBopomofo 使用 **Windows Named Pipe** 進行行程間通訊（IPC），連接 Client 端（TSF TIP）和 Server 端（輸入法核心引擎）。

- **Pipe 名稱**：`\\.\pipe\WinMcBopomofo_IPC_Pipe`
- **傳輸方式**：同步 Request-Response 模式
- **序列化格式**：換行分隔的文本格式（newline-delimited）

## 通訊架構

```
┌──────────────────┐                      ┌──────────────────┐
│  Client (TIP)    │                      │  Server (Engine) │
│  McBopomofoTIP   │◄─────────────────►   │  McBopomofoServer│
│   DLL Process    │   Named Pipe IPC     │   Background Svc │
└──────────────────┘                      └──────────────────┘
       ^                                            ^
       │                                            │
   Key Events                              State Updates
   Candidate Selection                     Composing Buffer
   Reset/Reload                            Candidates List
```

## 命令類型

### Command Enum

```cpp
enum class Command : int {
    CMD_RESET = 0,              // 重設輸入狀態
    CMD_KEY_EVENT = 1,          // 鍵盤事件
    CMD_SELECT_CANDIDATE = 2,   // 選擇候選字
    CMD_RELOAD_SETTINGS = 3,    // 重新載入設定
};
```

---

## 命令詳解

### 1. CMD_RESET (0) - 重設狀態

**用途**：清空 Server 的輸入狀態，提交 buffer 中的文字。

**觸發場景**：
- 按下 Ctrl + Space 切換中英文模式
- Client 端要求終止當前輸入

**Request 格式**：
```
0
```

**Response 格式**：
見 [StateUpdate 格式](#stateupdate-格式)

**Server 行為**：
```
1. 呼叫 controller.Reset()
   - 完成當前的輸入
   - 提交 buffer 中的文字（commitString）
   - 清空 composingBuffer
2. 更新狀態並返回
```

**Example**：
```
Request:  "0\n"
Response: "0\n0\n-1\n0\n-1\n-1\n5\n你好\n0\n\n0\n"
          (consumed=false, commitString="你好", composingBuffer="")
```

---

### 2. CMD_KEY_EVENT (1) - 鍵盤事件

**用途**：向 Server 發送鍵盤按鍵事件進行輸入處理。

**觸發場景**：
- 使用者按下任何鍵盤按鍵（在 IME 開啟的情況下）

**Request 格式**：
```
1
<VK_CODE>
<ASCII_CODE>
<SHIFT>
<CTRL>
```

| 欄位 | 類型 | 說明 |
|------|------|------|
| VK_CODE | unsigned int | Virtual Key Code（如 VK_SPACE = 32） |
| ASCII_CODE | unsigned int | ASCII 值（可選，打字時有效） |
| SHIFT | bool (0/1) | Shift 鍵是否按下 |
| CTRL | bool (0/1) | Ctrl 鍵是否按下 |

**Response 格式**：
見 [StateUpdate 格式](#stateupdate-格式)

**Server 行為**：
```
1. 將 Key Event 轉換為內部 Key 結構
2. 呼叫 controller.HandleKey(key)
3. 獲取新的輸入狀態
4. 返回 StateUpdate
```

**Examples**：

*Example 1: 按下 'a' 鍵*
```
Request:
1
65          (VK_A)
97          (ASCII 'a')
0           (SHIFT not pressed)
0           (CTRL not pressed)

Response:
1           (consumed=true)
1           (cursorIndex=1)
-1          (candidateIndex=-1)
0           (forceVertical=false)
-1          (markStart=-1)
-1          (markEnd=-1)
0           (commitString empty)
1           (composingBuffer size=1)
ㄚ
0           (tooltip empty)
0           (candidates count=0)
```

*Example 2: 按下 Ctrl+Space*
```
Request:
1
32          (VK_SPACE)
0           (ASCII)
0           (SHIFT not pressed)
1           (CTRL pressed)

Response:
(Server 會發送 RESET，而不是處理此按鍵)
(See: CMD_RESET)
```

*Example 3: 按下 Space（選字）*
```
Request:
1
32          (VK_SPACE)
32          (ASCII)
0           (SHIFT not pressed)
0           (CTRL not pressed)

Response:
0           (consumed=true)
3           (cursorIndex=3)
-1          (candidateIndex=-1)
0           (forceVertical=false)
-1          (markStart=-1)
-1          (markEnd=-1)
1           (commitString size=1)
你
0           (composingBuffer empty)
0           (tooltip empty)
0           (candidates count=0)
```

---

### 3. CMD_SELECT_CANDIDATE (2) - 選擇候選字

**用途**：選擇候選字視窗中指定索引的候選字。

**觸發場景**：
- 使用者按下數字鍵（1-9）選擇候選字
- 使用者用上下鍵瀏覽後按 Space/Enter

**Request 格式**：
```
2
<INDEX>
```

| 欄位 | 類型 | 說明 |
|------|------|------|
| INDEX | int | 候選字索引（0-based） |

**Response 格式**：
見 [StateUpdate 格式](#stateupdate-格式)

**Server 行為**：
```
1. 呼叫 controller.SelectCandidate(index)
2. 根據 index 選擇相應候選字
3. 返回更新的狀態
```

**Example**：
```
Request:
2
1           (選擇第 2 個候選字)

Response:
0           (consumed=true)
0           (cursorIndex=0)
-1          (candidateIndex=-1)
0           (forceVertical=false)
-1          (markStart=-1)
-1          (markEnd=-1)
2           (commitString size=2)
好的
0           (composingBuffer empty)
0           (tooltip empty)
0           (candidates count=0)
```

---

### 4. CMD_RELOAD_SETTINGS (3) - 重新載入設定

**用途**：通知 Server 重新載入設定檔案（mcbopomofo.ini）。

**觸發場景**：
- Settings UI 修改設定後保存
- Client 偵測到設定檔變更時

**Request 格式**：
```
3
```

**Response 格式**：
見 [StateUpdate 格式](#stateupdate-格式)

**Server 行為**：
```
1. 重新讀取 mcbopomofo.ini
2. 套用新設定到 controller
3. 返回當前狀態
```

**Example**：
```
Request:
3

Response:
0           (consumed=false)
0           (cursorIndex=0)
-1          (candidateIndex=-1)
0           (forceVertical=false)
-1          (markStart=-1)
-1          (markEnd=-1)
0           (commitString empty)
0           (composingBuffer empty)
0           (tooltip empty)
0           (candidates count=0)
```

---

## Response: StateUpdate 格式

所有命令都返回 **StateUpdate**，描述當前的輸入狀態。

**格式**：
```
<CONSUMED>
<CURSOR_INDEX>
<CANDIDATE_INDEX>
<FORCE_VERTICAL>
<MARK_START>
<MARK_END>
<COMMIT_STRING_SIZE>
<COMMIT_STRING>
<COMPOSING_BUFFER_SIZE>
<COMPOSING_BUFFER>
<TOOLTIP_SIZE>
<TOOLTIP>
<CANDIDATES_COUNT>
<CANDIDATE_1_SIZE>
<CANDIDATE_1>
<CANDIDATE_2_SIZE>
<CANDIDATE_2>
...
```

### 欄位說明

| 欄位 | 類型 | 說明 |
|------|------|------|
| CONSUMED | bool (0/1) | 該按鍵是否被 IME 消費（true = IME 處理，false = 傳給應用程式） |
| CURSOR_INDEX | int | composingBuffer 中的光標位置 |
| CANDIDATE_INDEX | int | 當前選中的候選字索引（-1 = 無選擇） |
| FORCE_VERTICAL | bool (0/1) | 是否強制候選字視窗垂直排列 |
| MARK_START | int | 標記的起始位置（-1 = 無標記） |
| MARK_END | int | 標記的結束位置 |
| COMMIT_STRING | string | 要提交給應用程式的文字 |
| COMPOSING_BUFFER | string | 目前的編輯中文字（注音或候選字） |
| TOOLTIP | string | 提示文字（例如「按 Space 選字」） |
| CANDIDATES_COUNT | int | 候選字個數 |
| CANDIDATE_N | string | 第 N 個候選字 |

### 字串編碼方式

**Sized String 格式**：
```
<SIZE_IN_BYTES>
<STRING_DATA>
```

- SIZE = 字串的 UTF-8 byte 數（不含終止符）
- 讀取 SIZE 個 bytes，然後必須讀一個 '\n'

**Example**：字串 "你好"
```
6           (UTF-8: "你" = 3 bytes, "好" = 3 bytes)
你好
```

---

## 序列化實現

### 基本原則

1. **行分隔**：所有元素以 '\n' 分隔
2. **字串長度前置**：變長字串使用長度 + 內容 + '\n' 格式
3. **布林值**：0 = false，1 = true
4. **編碼**：所有字串為 UTF-8

### C++ 實現

序列化和反序列化函數位置：
- 位置：`src/Common/Ipc.h`、`src/Common/Ipc.cpp`
- 函數：`Serialize*()` 和 `Deserialize*()`

## 通訊流程示例

### 典型的輸入流程

```
User types "你好"

1. User presses 'a'
   Client:  CMD_KEY_EVENT (VK=65, ASCII=97)
   Server:  StateUpdate (consumed=true, composingBuffer="ㄚ")
   Client:  Display "ㄚ" in composing area

2. User presses 'u'
   Client:  CMD_KEY_EVENT (VK=85, ASCII=117)
   Server:  StateUpdate (consumed=true, composingBuffer="ㄚㄨ", candidates=["你","..."])
   Client:  Display candidates window

3. User presses '1' to select first candidate
   Client:  CMD_SELECT_CANDIDATE (index=0)
   Server:  StateUpdate (consumed=true, composingBuffer="你")
   Client:  Update display

4. User presses 'e'
   Client:  CMD_KEY_EVENT (VK=69, ASCII=101)
   Server:  StateUpdate (consumed=true, composingBuffer="你ㄏㄜ", candidates=["好","..."])
   Client:  Update candidates

5. User presses '1' to select first candidate
   Client:  CMD_SELECT_CANDIDATE (index=0)
   Server:  StateUpdate (consumed=true, commitString="好", composingBuffer="")
   Client:  Commit "好" to application, clear composing area
```

### 模式切換流程

```
User presses Ctrl+Space (in Chinese mode)

1. Client detects Ctrl+Space in OnTestKeyDown()
   Client:  CMD_RESET
   Server:  Reset internal state, commit any pending text
            StateUpdate (commitString="你好", composingBuffer="")
   
2. Client toggles IME mode via TSF API
   TSF:     Change GUID_COMPARTMENT_KEYBOARD_OPENCLOSE to 0 (closed)
   
3. Langbar button text updates
   "中" → "英"
```

---

## 錯誤處理

### Deserialization 失敗

如果反序列化失敗：
- 返回 false
- Client 應記錄錯誤並嘗試重新連接
- Server 應忽略無效請求並返回當前狀態

### Pipe 連接失敗

**Client 端**：
- `NamedPipeClient::Call()` 返回 false
- 自動重試（通常由上層邏輯控制）
- 如果 Server 未運行，Connection 會失敗

**Server 端**：
- Named Pipe Server 在後台執行緒中運行
- 每個客戶端連接使用單個 Pipe 實例
- 連接關閉時自動清理

---

## 效能考量

### 最優化

1. **換行分隔**：簡單且快速的文本解析
2. **Named Pipe**：Windows 原生，效能優於 TCP/Socket
3. **同步 RPC**：請求-響應模式，無複雜的狀態管理

### 瓶頸

- 字串轉換（`stoi()`, `stoull()`）
- 每次按鍵都進行完整的序列化/反序列化
- Pipe 往返延遲（通常 < 1ms）

### 優化空間

- 使用二進位格式（而非文本）
- 批量操作（多個命令一次發送）
- 條件式更新（只發送改變的字段）

---

## 版本相容性

目前沒有版本號機制。如需修改 Protocol：

1. 添加新的 Command enum 值
2. 在 StateUpdate 中添加新字段（使用向後相容的方式）
3. 舊版本 Server 會忽略新字段

---

## 調試技巧

### 查看 IPC 訊息

Server 端有詳細的日誌輸出：
```
FCITX_MCBOPOMOFO_INFO() << "IPC Recv: VK=" << keyReq.vk << ", ...";
```

檢查日誌方式：
```
# 啟用 Debug 編譯
cmake -DCMAKE_BUILD_TYPE=Debug .
ctest --verbose
```

### 測試 IPC 連接

```cpp
// Test connection
McBopomofo::IPC::NamedPipeClient pipe(McBopomofo::IPC::PIPE_NAME);
std::string response;
if (pipe.Call(McBopomofo::IPC::SerializeReset(), response)) {
    McBopomofo::IPC::StateUpdatePayload state;
    McBopomofo::IPC::DeserializeStateUpdate(response, state);
    // Process state...
} else {
    // Connection failed
}
```

---

## 相關程式碼位置

| 項目 | 位置 |
|------|------|
| IPC 定義 | `src/Common/Ipc.h` / `Ipc.cpp` |
| Named Pipe | `src/Common/NamedPipe.h` / `NamedPipe.cpp` |
| Server 實現 | `src/Server/main.cpp` |
| Client 實現 | `src/Client/McBopomofoTIP.cpp` |
| Key 映射 | `src/Server/WindowsKeyBridge.cpp` |

---

## 常見問題

**Q: 為什麼使用文本格式而不是二進位？**
A: 易於調試、人可讀、跨平台編碼相容性好。效能差異在當前場景可接受。

**Q: Ctrl+Space 怎麼處理？**
A: Client 檢測到 Ctrl+Space 後，發送 CMD_RESET 命令給 Server。Server 執行 Reset 並提交 buffer 內容。

**Q: 如何添加新命令？**
A: 在 Command enum 中添加新值，實現序列化/反序列化函數，在 Server main.cpp 中添加處理邏輯。

**Q: StateUpdate 中的 -1 代表什麼？**
A: -1 通常表示「無值」或「不適用」，例如 candidateIndex=-1 表示沒有選中的候選字。

---

*文件最後更新：2026-05-09*
*版本：1.0（基於 Commit 無版本跟蹤）*
