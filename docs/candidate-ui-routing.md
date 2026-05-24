# Candidate UI Routing

這份文件說明 Win-McBopomofo 在 Windows TSF 環境下，什麼時候使用自訂選字窗 `CandidateWindow`，什麼時候使用 `ITfCandidateListUIElementBehavior` 實作的 `CCandidateListUIElement`，以及實際判斷分支在哪裡。

## 兩條候選 UI 路徑

專案內有兩條候選字顯示路徑：

1. 自訂視窗路徑
   `CandidateWindow` 是本專案自己畫的 popup window，負責視覺樣式、DPI 適應、定位與移動。

2. TSF UIElement 路徑
   `CCandidateListUIElement` 是提供給 `ITfUIElementMgr` 的標準 TSF candidate list 物件，讓宿主程式或系統用自己的方式呈現候選字。

這兩條路徑不是二選一地在啟動時固定決定，而是在每次 state update 進入 `CStateEditSession::DoEditSession()` 時依條件動態判斷。

## 元件建立時機

在 `McBopomofoTIP::ActivateEx()`：

- 一定會建立自訂 `CandidateWindow`
- 如果 `ITfUIElementMgr` 取得成功，另外建立 `CCandidateListUIElement`

對應程式位置：

- [src/Client/McBopomofoTIP.cpp](C:/Users/user/Works/win-mcbopomofo/src/Client/McBopomofoTIP.cpp:443)

這代表：

- `CandidateWindow` 幾乎總是可用
- `CCandidateListUIElement` 只有在宿主提供 `ITfUIElementMgr` 時才會參與判斷

## 高階判斷原則

候選字是否顯示，先看 `state_.candidates` 是否為空。

- 如果沒有候選字：兩條路徑都關閉
- 如果有候選字：先更新 TSF UIElement 路徑，再決定是否也顯示自訂 `CandidateWindow`

真正決定「自訂選字窗要不要顯示」的核心依據不是使用者設定，而是 `ITfUIElementMgr::BeginUIElement()` 回傳的 `bShow`。

程式把這個結果存進：

- `McBopomofoTIP::showCustomCandidateWindow_`

對應介面：

- [src/Client/McBopomofoTIP.h](C:/Users/user/Works/win-mcbopomofo/src/Client/McBopomofoTIP.h:170)

語意是：

- `bShow == TRUE`：宿主沒有接手顯示，TIP 應該自己顯示自訂 `CandidateWindow`
- `bShow == FALSE`：宿主或系統會處理 TSF candidate UI，TIP 不應再額外顯示自訂 `CandidateWindow`

## 實際分支流程

`CStateEditSession::DoEditSession()` 內有兩個入口會跑到候選字 UI 判斷：

1. 有 composing buffer 時
2. 沒有 composition，但仍有候選字或 tooltip 時

這兩段邏輯幾乎相同，只是觸發來源不同。第二段主要對應沒有組字中的候選清單情境，例如某些標點清單或獨立候選 UI。

對應程式位置：

- 第一段：[src/Client/StateEditSession.cpp](C:/Users/user/Works/win-mcbopomofo/src/Client/StateEditSession.cpp:458)
- 第二段：[src/Client/StateEditSession.cpp](C:/Users/user/Works/win-mcbopomofo/src/Client/StateEditSession.cpp:583)

判斷順序如下。

### 1. 沒有候選字

如果 `state_.candidates.empty()`：

- 隱藏 `CandidateWindow`
- 如果先前建立過 TSF UIElement，呼叫 `EndUIElement()`
- 把 `CCandidateListUIElement` 標成 not shown

對應程式位置：

- [src/Client/StateEditSession.cpp](C:/Users/user/Works/win-mcbopomofo/src/Client/StateEditSession.cpp:639)

### 2. 有候選字，且有 `ITfUIElementMgr`

如果下面兩個條件都成立：

- `pTIP_->GetUIElementMgr() != nullptr`
- `pTIP_->GetCandidateUIElement() != nullptr`

就先走 TSF UIElement 路徑。

流程是：

1. 呼叫 `CCandidateListUIElement::SetActiveContext()`
2. 呼叫 `CCandidateListUIElement::UpdateData()`，把候選字、選取索引、選字鍵等資料同步進 UIElement
3. 如果這是第一次顯示，呼叫 `BeginUIElement()`
4. 如果不是第一次，呼叫 `UpdateUIElement()`
5. 把 `BeginUIElement()` 回來的 `bShow` 存到 `showCustomCandidateWindow_`
6. 用 `showCustomCandidateWindow_` 決定是否還要顯示自訂 `CandidateWindow`

對應程式位置：

- `BeginUIElement()`：[src/Client/StateEditSession.cpp](C:/Users/user/Works/win-mcbopomofo/src/Client/StateEditSession.cpp:472)
- `UpdateUIElement()`：[src/Client/StateEditSession.cpp](C:/Users/user/Works/win-mcbopomofo/src/Client/StateEditSession.cpp:477)
- 第二個入口的同樣流程：[src/Client/StateEditSession.cpp](C:/Users/user/Works/win-mcbopomofo/src/Client/StateEditSession.cpp:597)

這裡的重點是：

- `CCandidateListUIElement` 永遠會先被更新
- `CandidateWindow` 是否顯示，要等 `BeginUIElement()` 的 `bShow` 決定

也就是說，`CCandidateListUIElement` 不是 `CandidateWindow` 的 fallback；它是先通知 TSF/宿主「我有 candidate list」，然後由宿主回覆是否還需要 TIP 顯示自己的 UI。

### 3. 有候選字，但沒有 `ITfUIElementMgr`

如果 `ITfUIElementMgr` 取不到，或 `CCandidateListUIElement` 沒建立成功：

- `showCustomCand` 會維持預設值 `true`
- 不會進入 TSF UIElement 流程
- 直接顯示自訂 `CandidateWindow`

對應程式位置可從 `showCustomCand` 的初始化看出：

- [src/Client/StateEditSession.cpp](C:/Users/user/Works/win-mcbopomofo/src/Client/StateEditSession.cpp:458)
- [src/Client/StateEditSession.cpp](C:/Users/user/Works/win-mcbopomofo/src/Client/StateEditSession.cpp:583)

這代表在不支援或不提供 TSF UIElement 管理的宿主上，專案會回退到自己的選字窗。

### 4. 顯示自訂 `CandidateWindow`

只有在以下條件同時成立時才會呼叫 `CandidateWindow::UpdateUI()`：

- `showCustomCand == true`
- `state_.candidates` 非空

對應程式位置：

- 第一個入口：[src/Client/StateEditSession.cpp](C:/Users/user/Works/win-mcbopomofo/src/Client/StateEditSession.cpp:504)
- 第二個入口：[src/Client/StateEditSession.cpp](C:/Users/user/Works/win-mcbopomofo/src/Client/StateEditSession.cpp:629)

如果 `showCustomCand == false`，就算本地有 candidate data，也只更新 TSF UIElement，不會顯示自訂 `CandidateWindow`。

### 5. 直接提交且沒有 composition 的特殊情況

如果這次 state update 是 direct commit，而且當前沒有 active composition：

- 先隱藏 `CandidateWindow`
- 先隱藏 tooltip
- 如果有 TSF UIElement，也一併 `EndUIElement()`
- 然後直接結束這次 edit session 的 UI 更新流程

對應程式位置：

- [src/Client/StateEditSession.cpp](C:/Users/user/Works/win-mcbopomofo/src/Client/StateEditSession.cpp:306)

這個分支的目的不是在選哪個 UI 路徑，而是避免 direct commit 後殘留任何候選 UI。

## `CCandidateListUIElement::Show()` 在這裡扮演什麼角色

`CCandidateListUIElement::Show(BOOL fShow)` 只會更新 UIElement 內部的 `fShown_` 狀態。

對應程式位置：

- [src/Client/TsfUiElement.cpp](C:/Users/user/Works/win-mcbopomofo/src/Client/TsfUiElement.cpp:120)

目前自訂 `CandidateWindow` 的顯示與否，不是直接根據 `CCandidateListUIElement::Show()` 的 `fShown_` 來判斷，而是根據 `BeginUIElement()` 回傳的 `bShow`，也就是 `showCustomCandidateWindow_`。

因此可以把兩者分開理解：

- `CCandidateListUIElement::Show()`：TSF UIElement 自己的 shown state
- `showCustomCandidateWindow_`：TIP 是否應該另外顯示自訂選字窗

## 總結成決策表

| 條件 | TSF `CCandidateListUIElement` | 自訂 `CandidateWindow` |
| --- | --- | --- |
| `state_.candidates` 為空 | 關閉 / EndUIElement | 隱藏 |
| 有 candidates，且沒有 `ITfUIElementMgr` | 不可用 | 顯示 |
| 有 candidates，`BeginUIElement()` 成功且 `bShow == TRUE` | 更新 | 顯示 |
| 有 candidates，`BeginUIElement()` 成功且 `bShow == FALSE` | 更新 | 不顯示 |
| direct commit without composition | 關閉 / EndUIElement | 隱藏 |

## 一句話版

實作上的真實規則是：

先把 candidate data 提供給 TSF UIElement；如果宿主透過 `BeginUIElement()` 表示「不用你自己畫」，就只用 `CCandidateListUIElement` 路徑；否則再顯示本專案的 `CandidateWindow`。
