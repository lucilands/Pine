#ifndef _PXW32_H
#define _PXW32_H

#include <windows.h>

typedef HGLRC (WINAPI * PFNWGLCREATECONTEXTATTRIBSARBPROC) (HDC hDC, HGLRC hShareContext, const int *attribList);

typedef struct {
    HINSTANCE hinst;
    WNDCLASS wc;
} context_t;

typedef struct {
    HWND window_handle;
    MSG msg;
    HDC device_context;
    HGLRC gl_loadctx;
    PFNWGLCREATECONTEXTATTRIBSARBPROC gl_loadproc;
    HGLRC gl_ctx;
} window_t;

#endif //_PXW32_H