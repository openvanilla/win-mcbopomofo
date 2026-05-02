# McBopomofo Windows TSF 移植技術評估報告

## 1. 目標 (Objective)
將 McBopomofo (小麥注音) 輸入法移植至 Windows 平台，基礎開發環境為 **Visual Studio 2022**，架構採用 **Windows Text Services Framework (TSF)**，並達成核心引擎在全系統中僅有**單一實例 (Single Instance)** 的目標。

## 2. 架構分析 (Architectural Analysis)

### 2.1 TSF 的限制
Windows 的 TSF TIP (Text Input Processor) 本質上是 COM DLL。當應用程式取得焦點時，系統會將此 DLL 載入該程序 (Process) 的記憶體空間。
- **問題**：這會導致每個開啟的應用程式（如 Notepad, Chrome, Word）都擁有一個獨立的輸入法實例，無法自然達成「單一實例」的要求。
- **影響**：巨大的語言模型 (Language Model) 會在每個程序中重複載入（佔用數十至數百 MB 記憶體），且使用者學習記錄難以即時同步。

### 2.2 解決方案：Client/Server (Service-Based) 架構
為了達成單一實例目標，建議將輸入法拆分為兩個組件：

#### A. Server (背景服務/程序) - 「大腦」
- **角色**：核心邏輯中心。
- **職責**：
    - 載入並管理語言模型 (Language Model)。
    - 管理使用者辭典與偏好設定。
    - 維護每個 Client Session 的輸入狀態 (Input State)。
    - 執行 `KeyHandler` 與 `Gramambular2` 演算法。
- **優點**：全系統僅載入一份模型，節省記憶體，並確保學習記錄即時同步。

#### B. Client (TSF TIP DLL) - 「感知與顯示層」
- **角色**：輕量級代理人。
- **職責**：
    - 實作 `ITfTextInputProcessorEx` 以與系統整合。
    - 攔截鍵盤事件 (`ITfKeyEventSink`) 並傳送至 Server。
    - 接收 Server 回傳的狀態，更新 TSF Composition String 與渲染候選字清單 (Candidate List)。
- **技術**：透過高速 IPC (Inter-Process Communication) 與 Server 通訊。

## 3. 技術選型 (Technical Stack)

- **開發環境**：Visual Studio 2022。
- **程式語言**：C++20 (充分利用現代 C++ 特性並與現有核心相容)。
- **核心引擎**：移植自 `fcitx5-mcbopomofo/src/Engine`。
- **通訊協定 (IPC)**：
    - **Named Pipes (具名管道)**：提供跨 AppContainer (如 Edge, UWP) 的通訊能力。
    - **序列化**：使用自定義 struct 或 Protobuf/FlatBuffers。
- **UI 渲染**：使用 **DirectWrite + Direct2D**。
    - **理由**：相比傳統 GDI，DirectWrite 能完美支援彩色 Emoji，且在 High DPI 縮放下的字體渲染品質更佳。
- **主題管理**：實作動態色盤切換，監聽系統主題 (Light/Dark mode) 事件。

## 4. 移植挑戰與應對

### 4.1 核心移植 (Core Engine Porting)
...

### 4.4 High DPI 與多螢幕支援
- **應對**：Client DLL 需宣告為 Per-Monitor DPI Aware V2。候選字視窗在移動至不同解析度螢幕時，需透過 `WM_DPICHANGED` 動態計算字體與間距。

### 4.5 現代化 UI 特性
- **Emoji**：選字窗需確保能正確顯示 Unicode 繪文字，這要求渲染層必須處理 `Surrogate Pairs` 並選用正確的字體 (如 Segoe UI Emoji)。
- **佈局切換**：候選字視窗需支援橫向 (Horizontal) 與縱向 (Vertical) 兩種佈局，這需要在 UI 邏輯中抽離佈局管理器。
- **深色模式**：監聽 `WM_SETTINGCHANGE` 並讀取 `AppsUseLightTheme` 登錄檔值，即時更新選字窗的背景色與文字顏色。

### 4.6 無縫更新與檔案鎖定 (File Locking)
- **挑戰**：TSF Client DLL 會被每個執行的應用程式載入並鎖定，這導致安裝程式在進行升級更新時會遭遇檔案無法覆寫的問題，通常需要強制使用者重新開機。
- **對策**：
    1.  **極簡化 Client DLL (Thin Client)**：將邏輯與 UI 盡可能移至 Server 端，使 Client DLL 的程式碼變動頻率降至最低。大部分的版本更新只需替換 Server EXE 與詞庫檔案。
    2.  **重新命名替換機制**：當必須更新 Client DLL 時，安裝程式需先將舊 DLL 重新命名（如 `.old`），這在 Windows 中是被允許的，接著再將新版 DLL 放入。系統會利用 `PendingFileRenameOperations` 於下次重開機時清理舊檔，而使用者在安裝當下不會被中斷。
    3.  **Server 優雅關閉**：更新前透過 IPC 傳送訊號要求 Server 安全退出，替換 Server EXE 後再重新啟動，無需影響正在運行的應用程式。

### 4.7 系統註冊機制 (System Registration)
- **挑戰**：輸入法必須在系統中正確登記才能被使用者選用，這涉及 COM 註冊與 TSF Profile 註冊。
- **對策**：
    1.  **COM 註冊**：Client DLL 需實作 `DllRegisterServer` 與 `DllUnregisterServer`，負責寫入 CLSID 與 InprocServer32 等登錄檔資訊。
    2.  **TSF Profile 註冊**：安裝程式或註冊工具需呼叫 `ITfInputProcessorProfiles::Register` 與 `ITfInputProcessorProfiles::AddLanguageProfile`，將輸入法掛載至「中文 (台灣)」(0x0404) 語系下。
    3.  **類別管理**：透過 `ITfCategoryMgr` 將 TIP 標記為 `GUID_TFCAT_TIP_KEYBOARD`，使其出現在鍵盤選單中。
### 4.8 設定管理與持久化 (Configuration & Persistence)
- **設定格式**：採用 **INI 檔案** (`.ini`)。
    - **對策**：使用 Win32 API 處理設定讀寫，確保設定檔易於手動檢查與跨版本相容。
- **使用者詞庫**：採用 **純文字格式** (`.txt`)。
    - **對策**：每行格式為 `詞彙 讀音` (例如：`小麥 ㄒㄧㄠˇ-ㄇㄞˋ`)。Server 需實作 File Watcher，當文字檔變動時自動重新載入。
- **獨立設定程式 (Config App)**：
    - **對策**：開發一個獨立的 Win32 GUI 程序 (`McBopomofoConfig.exe`)。
    - **職責**：提供圖形介面修改 INI 設定、編輯純文字詞庫，並在儲存後通知 Server 重新載入設定。

### 4.9 檔案佈局與權限規範 (File Layout & Permissions)
- **挑戰**：必須符合 Windows UAC 規範，區分全系統執行的程式檔案與個別使用者的設定資料。
- **對策**：
    1.  **安裝路徑**：所有執行檔與靜態資料安裝至 `%ProgramFiles%\OpenVanilla\McBopomofo` (需要管理員權限寫入)。包含：
        - `McBopomofoTIP.dll` (Client)
        - `McBopomofoServer.exe` (Server)
        - `McBopomofoConfig.exe` (獨立設定工具)
        - 系統語言模型檔案
    2.  **使用者資料路徑**：使用者的動態資料儲存於 `%APPDATA%\OpenVanilla\McBopomofo` (無需特權即可讀寫)。包含：
        - `config.ini` (偏好設定)
        - `user_phrases.txt` (自訂詞庫與學習記錄)

## 5. 結論

採用 **Client/Server 架構** 是達成您「單一實例」要求的唯一且最佳路徑。此架構不僅能顯著降低記憶體消耗，還能提供最一致的使用者體驗。
