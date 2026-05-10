#pragma once
#include <msctf.h>
#include <windows.h>

extern HINSTANCE g_hInst;
extern LONG g_cRefDll;

void DllAddRef();
void DllRelease();

void LogMessage(const char* format, ...);
