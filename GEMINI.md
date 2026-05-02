# Win-McBopomofo 開發規範

## 核心原則
*   **核心引擎保護**：嚴禁修改 `src/Engine` 及其相關的核心演算法代碼（如 `gramambular2`）。
*   **適配與橋接**：所有針對 Windows 平台（TSF, Win32 API）的調整，應實作在適配層（Adapter/Bridge Layer）中，不得侵入核心邏輯。
*   **語言規範**：使用 C++20 標準。
*   **溝通規範**：對話與文件撰寫均使用**繁體中文**。
