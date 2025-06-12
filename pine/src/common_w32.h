#ifndef _PXW32_H
#define _PXW32_H

#include <windows.h>

typedef struct {
    HINSTANCE hinst;
    WNDCLASS wc;
} context_t;

typedef struct {
    HWND window_handle;
    MSG msg;
} window_t;

#endif //_PXW32_H