#include <pine.h>
#include <windows.h>

#ifdef PX_INCLUDE_STDLIB
#include <stdlib.h>
#endif // PX_INCLUDE_STDLIB

#ifdef PX_INCLUDE_STRING
#include <string.h>
#endif // PX_INCLUDE_STRING

#ifdef PX_INCLUDE_STDDEF
#include <stddef.h>
#endif // PX_INCLUDE_STDDEF

#include <stdio.h>

#include "common.h"
#include "common_w32.h"

LRESULT CALLBACK wnd_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    PxWindow *self = (PxWindow*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    switch (uMsg)
    {
        case WM_NCCREATE:
            return PX_TRUE;

        case WM_DESTROY:
            PostQuitMessage(0);
            return PX_FALSE;
            
        case WM_CLOSE:
            self->should_close = PX_TRUE;
            return PX_FALSE;
            
        case WM_SIZE:
            self->info.width = LOWORD(lParam);
            self->info.height = HIWORD(lParam);
            break;
        
        case WM_MOVE:
            self->info.x = LOWORD(lParam);
            self->info.y = HIWORD(lParam);
            break;

        default: break;
    }

    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

PxContext *PxCreateContext(PxResult *res) {
    PxContext *ret = PxMalloc(sizeof(struct PxContext_internal));
    ERRCHECK_N(ret, *res, PX_FAILED_ALLOC);

    PxMemset(ret, 0, sizeof(struct PxContext_internal));

    ret->inner = PxMalloc(sizeof(context_t));
    context_t *con = ((context_t*)ret->inner);

    con->hinst = GetModuleHandleA(NULL);
    ERRCHECK_N(con->hinst, *res, PX_FAILED_HANDLE_FETCH);

    con->wc = (WNDCLASS){0};
    con->wc.lpfnWndProc = (WNDPROC)wnd_proc;
    con->wc.hInstance = con->hinst;
    con->wc.lpszClassName = PX_WNDCLS_NAME;

    RegisterClassA(&con->wc);
    ret->result = res;

    return ret;
}

PxWindow *PxCreateWindow(PxContext *context, const PxWindowInfo info, PxWindow *parent) {
    PxWindow *ret = PxMalloc(sizeof(struct PxWindow_internal));
    ERRCHECK_N(ret, *context->result, PX_FAILED_ALLOC);

    PxMemset(ret, 0, sizeof(struct PxWindow_internal));

    ret->inner = PxMalloc(sizeof(window_t));
    ERRCHECK_N(ret->inner, *context->result, PX_FAILED_ALLOC);
    PxMemset(ret->inner, 0, sizeof(window_t));

    window_t *win = ((window_t*)ret->inner);

    HWND hparent = NULL;
    if (parent) hparent = ((window_t*)parent->inner)->window_handle;

    win->window_handle = CreateWindowExA(
        WS_EX_LEFT, // Style
        PX_WNDCLS_NAME,
        info.title, // Title
        WS_OVERLAPPEDWINDOW,

        info.x, info.y,
        info.width, info.height,

        hparent, // Possible parent
        NULL,
        ((context_t*)context->inner)->hinst,
        NULL
    );
    ERRCHECK_N(win->window_handle, *context->result, PX_FAILED_WINDOW_CREATION)
    SetWindowLongPtrA(win->window_handle, GWLP_USERDATA, (LONG_PTR)ret);
    
    ShowWindow(win->window_handle, SW_NORMAL);

    SetWindowTextA(win->window_handle, info.title);
    UpdateWindow(win->window_handle);
    ret->should_close = PX_FALSE;

    ret->info = (PxWindowInfo){
        .width = info.width,
        .height = info.height,
        
        .x = info.x,
        .y = info.y,

        .title = PxMalloc(strlen(info.title)+1)
    };
    ERRCHECK_N(ret->info.title, *context->result, PX_FAILED_ALLOC)
    strcpy(ret->info.title, info.title);

    return ret;
}

void PxPollEvents(PxWindow *window) {
    GetMessageA(&((window_t*)window->inner)->msg, NULL, 0, 0);
    TranslateMessage(&((window_t*)window->inner)->msg);
    DispatchMessageA(&((window_t*)window->inner)->msg);
}
