#pragma once
#include <windows.h>
#include <msctf.h>

extern HINSTANCE g_hInst;
extern LONG g_cRefDll;

void DllAddRef();
void DllRelease();
