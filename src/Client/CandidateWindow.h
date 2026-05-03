#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <d2d1.h>
#include <dwrite.h>

class CandidateWindow {
public:
    CandidateWindow();
    ~CandidateWindow();

    bool Create(HINSTANCE hInstance);
    void Destroy();

    void UpdateUI(const std::vector<std::string>& candidates, int cursorIndex, bool forceVertical = false);                                                                                                                    
    void Move(int x, int y);
    void Hide();

    // For testing purposes
    std::wstring GetDisplayString() const { return _displayString; }
private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT OnPaint(HWND hwnd);
    void OnSettingChange();

    void CreateDeviceIndependentResources();
    void CreateDeviceResources();
    void DiscardDeviceResources();
    void UpdateTheme();
    float GetDpiScale();

    struct TextRange {
        UINT32 start;
        UINT32 length;
    };

    HWND _hwnd;
    float _dpiScale;
    std::vector<std::wstring> _candidates;
    int _cursorIndex;
    std::wstring _displayString;
    bool _isVertical;
    bool _isDarkMode;

    TextRange _selectedRange;
    std::vector<TextRange> _keyRanges;

    ID2D1Factory* _pD2DFactory;
    ID2D1HwndRenderTarget* _pRenderTarget;
    IDWriteFactory* _pDWriteFactory;
    IDWriteTextFormat* _pTextFormat;
    IDWriteTextLayout* _pTextLayout;

    ID2D1SolidColorBrush* _pTextBrush;
    ID2D1SolidColorBrush* _pBgBrush;
    ID2D1SolidColorBrush* _pBorderBrush;
    ID2D1SolidColorBrush* _pHighlightBgBrush;
    ID2D1SolidColorBrush* _pHighlightTextBrush;
};
