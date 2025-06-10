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

#define ERRCHECK_N(var_to_check, res, errcode) if (!(var_to_check)) {(res) = (errcode); return NULL;}
#define ERRCHECK_V(var_to_check, res, errcode) if (!(var_to_check)) {(res) = (errcode); return;}

#endif //_PXW32_H