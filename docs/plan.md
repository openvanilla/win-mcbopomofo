# Win-McBopomofo 開發計畫

此專案旨在為 Windows 平台開發一個基於 McBopomofo (小麥注音) 核心的輸入法。我們將參考現有的 macOS、Linux (fcitx5) 以及 Web 版本實作進行移植。

## 1. 專案架構 (Client/Server Model)

為了達成全系統單一實例 (Single Instance) 並有效管理記憶體，本專案採用 Client/Server 架構：

### Server (背景服務 / Service Process)
*   **角色**：輸入法核心引擎。
*   **功能**：
    *   **核心引擎 (Gram Engine)**：移植自 `fcitx5-mcbopomofo`，處理注音組合、語言模型分析。
    *   **資料管理**：載入系統詞庫 (LM)，管理使用者詞庫與偏好設定。
    *   **通訊管理**：透過 IPC (Named Pipes) 與多個 Client 實例通訊。
*   **單一性**：全系統僅啟動一個 Server 程序。

### Client (TSF Text Service DLL)
*   **角色**：Windows 系統介面代理。
*   **技術**：使用 Windows Text Services Framework (TSF)。
*   **功能**：
    *   **事件攔截**：透過 `ITfKeyEventSink` 獲取按鍵並傳送至 Server。
    *   **狀態同步**：接收 Server 回傳的 InputState，更新 `ITfComposition`。
    *   **UI 渲染**：顯示候選字視窗 (Candidate Window)。

### 設定與管理工具 (Preference Tool)
*   **型態**：獨立執行檔 (`McBopomofoConfig.exe`)。
*   **功能**：管理 INI 設定與純文字格式的使用者詞庫。

## 2. 技術選型

*   **開發環境**：Visual Studio 2022。
*   **程式語言**：C++ 20。
*   **作業系統介面**：Windows TSF, Win32 API。
*   **資料格式**：INI (設定), 純文字 (詞庫)。
*   **通訊方式 (IPC)**：Named Pipes (具名管道)。
*   **建置系統**：CMake。

## 3. 開發階段 (Milestones)

### 第一階段：環境搭建與核心移植
 1.  建立專案結構 (CMake)。
 2.  引入 McBopomofo 的 C++ 核心代碼與詞庫（位於 `src/Engine`）。
 3.  **核心移植規範**：
     *   **嚴禁修改核心引擎 (Core Engine)**：必須保持 `src/Engine` 目錄及相關演算法代碼的原樣。
     *   **適配層開發**：僅允許修改或新增高層 API 適配層（如 `KeyHandler` 的包裝或 TSF 介面橋接），以對接 Windows 平台的 IME API。
 4.  編譯並測試核心引擎在 Windows 上的運作。
### 第二階段：TSF 輸入法框架實作
1.  實作 TSF 基本介面，讓 Windows 能識別並載入此輸入法 DLL。
2.  實作 `ITfKeyEventSink`，處理基本的按鍵輸入。
3.  實作組合字串 (Composition String) 的顯示邏輯。

### 第三階段：候選字視窗與互動 (Modern UI)
1.  **Direct2D 渲染引擎**：實作基於 DirectWrite / Direct2D 的渲染層，確保支援彩色 Emoji。
2.  **DPI 適配**：實作 Per-Monitor DPI V2 支援，監聽 `WM_DPICHANGED`。
3.  **多樣化佈局**：實作橫向與縱向選字窗佈局邏輯。
4.  **主題切換**：整合系統 Light / Dark mode 色盤。
5.  處理候選字選擇與確認 (Commit)。

### 第四階段：進階功能與安裝佈署
1.  使用者詞庫 (User Phrases) 的讀寫支援。
2.  實作設定介面。
3.  **安裝程式與系統註冊**：
    *   實作 Client DLL 的 COM 註冊邏輯 (`DllRegisterServer`)。
    *   開發註冊工具，呼叫 TSF API 進行語系與類別註冊。
    *   撰寫安裝指令碼 (如 Inno Setup)，處理檔案複製與伺服器重啟。
4.  處理特殊輸入模式（如：標點符號、符號輸入）。
5.  效能優化與相容性測試。

## 4. 待決事項與挑戰
*   **TSF 複雜度**：TSF 的 API 較為複雜，需要處理多種邊界情況（如：Focus 變換、不同應用程式的相容性）。
    *   *對策*：參考開源專案 **Chewing (新酷音)** 與 **PIME** 的 TSF 實作，特別是 PIME 的 Client/Server 架構與安裝/註冊腳本。
*   **IPC 效能**：確保 Client/Server 間的通訊延遲低於 10ms，不影響打字流暢度。
*   **UI 渲染**：確保在不同 DPI 設定下，候選字視窗的縮放、定位與 Emoji 顯示皆能完美運作。
*   **無縫安裝與更新 (Seamless Update)**：TSF DLL 會被執行中的應用程式鎖定。設計上需極簡化 Client DLL 以降低更新頻率，並在安裝程式端實作「重新命名舊檔 (Pending Rename) + 取代新檔」的機制，確保使用者更新時不被強制中斷或要求立即重開機。
