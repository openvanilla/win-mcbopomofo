# 候選模式與空白鍵行為

## 目的

本文檔說明目前 Windows 版對空白鍵的實際定義，避免把兩件不同的事情混在一起：

- 空白鍵是否用來「進入候選模式」
- 進入候選模式後，空白鍵是否用來「翻頁」

## 結論

目前系統的語意如下：

1. `ChooseCandidateUsingSpace` 設定控制的是：
   是否允許在一般輸入狀態下，用空白鍵進入候選模式。
2. 一旦已經進入候選模式：
   空白鍵的意義固定為「候選列表下一頁」。

也就是說，這個設定不是「空白鍵是否在候選模式中選字」，而是「空白鍵是否從一般輸入狀態切入候選模式」。

## 一般輸入狀態下的空白鍵

實作位於 `src/Server/KeyHandler.cpp`。

- 若目前 state 是 `NotEmpty`，且：
  - 使用者按下 `Shift+Space`，或
  - `ChooseCandidateUsingSpace == false`
- 則空白鍵被當作實際空白字元插入 composing buffer。

相對地：

- 若目前 state 是 `NotEmpty`
- 且 `ChooseCandidateUsingSpace == true`
- 且 reading 為空

則按下空白鍵會進入 candidate choosing state。

## 候選模式下的空白鍵

實作位於 `src/Server/InputController.cpp` 的 `HandleCandidateKey()`。

當目前狀態屬於 candidate state，例如：

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

按下空白鍵會執行：

- `MoveCandidatePage(true)`

也就是翻到下一頁，而不是直接選取目前候選字。

## 與 fcitx5 版本的對照

fcitx5 版同樣把「一般輸入狀態的空白鍵」與「候選模式中的空白鍵」視為不同層次的邏輯：

- 一般輸入狀態由 `KeyHandler` 決定是否用空白鍵切入 candidate choosing
- 進入 candidate panel 後，候選列表按鍵處理由候選模式邏輯與框架 UI 負責

Windows 版先前的問題不是缺少 `Space -> page down` 的邏輯，而是翻頁後沒有立即呼叫 UI update，因此畫面看起來像沒有翻頁。這個問題已在 `InputController::HandleCandidateKey()` 補上 `ui_->Update(...)`。

## 對設定名稱的提醒

目前設定畫面顯示的是：

- `使用空白鍵選取候選字`

但依照實際程式行為，這個文字並不精確。更接近實作的描述是：

- `使用空白鍵進入候選模式`

或：

- `空白鍵用於候選模式切入`

若未來要改善使用者理解，建議優先修改 UI 文案，而不是修改這個設定的底層語意。
