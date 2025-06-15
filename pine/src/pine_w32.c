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
#include <assert.h>

#include "common.h"
#include "common_w32.h"

LRESULT CALLBACK wnd_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    PxWindow *self = (PxWindow*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    PxEvent event = {0};
    switch (uMsg)
    {
        case WM_NCCREATE: return PX_TRUE;
        
        case WM_SIZE:
            self->info.width = LOWORD(lParam);
            self->info.height = HIWORD(lParam);
            event.type = PX_EVENT_RESIZE;
            PxiPushEventStack(self->ctx, &self->ecache, event);
            break;
        
        case WM_MOVE:
            self->info.x = LOWORD(lParam);
            self->info.y = HIWORD(lParam);
            event.type = PX_EVENT_WINDOW_MOVE;
            PxiPushEventStack(self->ctx, &self->ecache, event);
            break;

        case WM_KEYDOWN:
            event.type = PX_EVENT_KEYDOWN;
            event.keycode = (int)wParam;
            PxiPushEventStack(self->ctx, &self->ecache, event);
            break;

        case WM_KEYUP:
            event.type = PX_EVENT_KEYUP;
            event.keycode = (int)wParam;
            PxiPushEventStack(self->ctx, &self->ecache, event);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            return PX_FALSE;
            
        case WM_CLOSE:
            self->should_close = PX_TRUE;
            event.type = PX_EVENT_CLOSE;
            PxiPushEventStack(self->ctx, &self->ecache, event);
            return PX_FALSE;

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

    ret->ecache = PxiCreateEventStack(context);
    ERRCHECK_N(context->result, context->result, context->result);

    return ret;
}

int PxPollEvents(PxWindow *window, PxEvent *event) {
    window_t *iwin = (window_t*)window->inner;
    event->type = __PX_EVENT_NONE__;
    
    if (window->ecache.len) *event = PxiPopEventStack(&window->ecache);
    if (PeekMessageA(&iwin->msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&iwin->msg);
        DispatchMessageA(&iwin->msg);
        if (window->ecache.len) *event = PxiPopEventStack(&window->ecache);
        return PX_TRUE;
    }
    
    return PX_FALSE;
}

PxDisplayInfo PxGetDisplay(PxContext *context) {
    int num_monitors = GetSystemMetrics(SM_CMONITORS);
    ERRCHECK_T(num_monitors, *context->result, PX_FAILED_OSCALL, PxDisplayInfo);

    int width = GetSystemMetrics(SM_CXSCREEN);
    ERRCHECK_T(width, *context->result, PX_FAILED_OSCALL, PxDisplayInfo);

    int height = GetSystemMetrics(SM_CYSCREEN);
    ERRCHECK_T(height, *context->result, PX_FAILED_OSCALL, PxDisplayInfo);

    return (const PxDisplayInfo) {
        width, height,
        num_monitors
    };
}