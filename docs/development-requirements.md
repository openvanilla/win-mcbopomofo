# 開發要求

## 1. TDD 要求

本專案採用 TDD（Test-Driven Development）作為預設開發方式。

新增功能、修正行為或調整狀態機時，請遵守以下順序：

1. 先寫測試。
2. 讓測試能描述預期行為，並在修改前能重現失敗。
3. 再修改正式程式碼。
4. 以測試通過作為該變更完成的最低標準。

換句話說：

- 沒有測試的功能，不算完成。
- 測試沒有通過的功能，不算完成。

## 2. 測試檔案位置

所有專案自有測試都放在 `tests/` 目錄下。

目前已存在的測試例如：

- `tests/BugReproTest.cpp`
- `tests/IpcTest.cpp`
- `tests/test_candidate_window.cpp`

若新增測試：

- 請放在 `tests/` 目錄下
- 並同步更新根目錄 `CMakeLists.txt`，把測試 target 與 `add_test(...)` 補上

## 3. 測試命名與範圍

建議依功能切分測試，而不是把大量不相干案例堆在同一檔案中。

例如：

- candidate window 顯示邏輯
- IPC encode/decode
- 特定 bug repro
- state transition

每個測試應該盡量只驗證一個明確行為。

## 4. 如何執行測試

### 建置單一測試

在既有 build 目錄下：

```powershell
cmake --build . --config Debug --target BugReproTest
```

或：

```powershell
cmake --build . --config Debug --target CandidateWindowTest
cmake --build . --config Debug --target IpcTest
```

### 執行單一測試

```powershell
ctest -C Debug -R BugReproTest --output-on-failure
```

### 執行專案自有測試

```powershell
ctest -C Debug --output-on-failure
```

目前根目錄 CMake 已刻意關閉第三方依賴（例如 OpenCC 子專案）自己的測試註冊，因此這個指令應只跑本專案維護的測試。

若某些環境下 `ctest` 本身對 Windows 可執行檔路徑解析異常，也可以直接執行測試程式：

```powershell
.\build_verify\bin\Debug\CandidateWindowTest.exe
.\build_verify\bin\Debug\BugReproTest.exe
.\build_verify\bin\Debug\IpcTest.exe
```

## 5. 完成定義

一個功能或修正要算完成，至少需要滿足：

1. 有對應測試。
2. 測試放在 `tests/` 目錄下。
3. 測試已加入 CMake。
4. 測試在本機 build 中通過。
5. 若變更影響既有行為，相關舊測試也必須保持通過。

## 6. 建議實務

### 新增功能時

- 先寫失敗中的測試
- 再做最小必要修改
- 最後執行受影響測試

### 修 bug 時

- 先把 bug 重現成 regression test
- 確認修正前失敗
- 修正後確認通過

### 修改 state machine 時

- 優先補：
  - state transition test
  - candidate paging / selection test
  - commit vs composing behavior test

這類修改如果只靠手動驗證，很容易再次引入 Notepad、candidate list、direct commit 類型的回歸問題。
