#pragma once
#include <windows.h>
#include <string>
#include <d2d1.h>
#include <dwrite.h>

class TooltipWindow {
public:
    TooltipWindow();
    ~TooltipWindow();

    bool Create(HINSTANCE hInstance);
    void Destroy();

    void UpdateUI(const std::string& tooltipText);
    void Move(int x, int y);
    void Hide();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT OnPaint(HWND hwnd);

    void CreateDeviceIndependentResources();
    void CreateDeviceResources();
    void DiscardDeviceResources();
    float GetDpiScale();

    HWND _hwnd;
    float _dpiScale;
    std::wstring _displayString;

    ID2D1Factory* _pD2DFactory;
    ID2D1HwndRenderTarget* _pRenderTarget;
    IDWriteFactory* _pDWriteFactory;
    IDWriteTextFormat* _pTextFormat;
    IDWriteTextLayout* _pTextLayout;

    ID2D1SolidColorBrush* _pTextBrush;
    ID2D1SolidColorBrush* _pBgBrush;
    ID2D1SolidColorBrush* _pBorderBrush;
};
