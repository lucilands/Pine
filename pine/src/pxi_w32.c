#include <pine.h>

#include "common.h"
#include "common_w32.h"

void PxiUpdatePosition(PxContext *context, PxWindow *window, int *new_position) {
    window->info.x = new_position[0];
    window->info.y = new_position[1];
    WINBOOL ret = SetWindowPos(((window_t*)window->inner)->window_handle,
        HWND_TOP,
        window->info.x, window->info.y,
        0, 0,
        SWP_NOSIZE | SWP_SHOWWINDOW
    );
    ERRCHECK_V(ret, *context->result, PX_FAILED_OSCALL);
}

void PxiUpdateSize(PxContext *context, PxWindow *window, int *new_size) {
    window->info.width = new_size[0];
    window->info.height = new_size[1];
    WINBOOL ret = SetWindowPos(((window_t*)window->inner)->window_handle,
        HWND_TOP,
        0, 0,
        window->info.width, window->info.height,
        SWP_NOMOVE | SWP_SHOWWINDOW
    );
    ERRCHECK_V(ret, *context->result, PX_FAILED_OSCALL);
}

void PxiUpdateRect(PxContext *context, PxWindow *window, int *new_rect) {
    window->info.width = new_rect[0];
    window->info.height = new_rect[1];
    window->info.x = new_rect[0];
    window->info.y = new_rect[1];
    WINBOOL ret = SetWindowPos(
        ((window_t*)window->inner)->window_handle,
        HWND_TOP,
        window->info.x, window->info.y,
        window->info.width, window->info.height,
        SWP_SHOWWINDOW
    );
    ERRCHECK_V(ret, *context->result, PX_FAILED_OSCALL);
}

void PxiUpdateTitle(PxContext *context, PxWindow *window, const char *new_title) {
    window->info.title = realloc(window->info.title, strlen(new_title)+1);
    ERRCHECK_V(window->info.title, *context->result, PX_FAILED_ALLOC);
    strcpy(window->info.title, new_title);

    SetWindowTextA(((window_t*)window->inner)->window_handle, window->info.title);
}