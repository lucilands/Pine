#include <pine.h>

#include "common.h"
#include "common_w32.h"

void PxiUpdateX(PxContext *context, PxWindow *window, int new_x) {
    window->info.x = new_x;
    WINBOOL ret = SetWindowPos(((window_t*)window->inner)->window_handle, HWND_TOP, window->info.x, window->info.y, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
    ERRCHECK_V(ret, *context->result, PX_FAILED_OSCALL);
}

void PxiUpdateY(PxContext *context, PxWindow *window, int new_y) {
    window->info.y = new_y;
    WINBOOL ret = SetWindowPos(((window_t*)window->inner)->window_handle, HWND_TOP, window->info.x, window->info.y, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
    ERRCHECK_V(ret, *context->result, PX_FAILED_OSCALL);
}

void PxiUpdateWidth(PxContext *context, PxWindow *window, int new_width) {
    window->info.width = new_width;
    WINBOOL ret = SetWindowPos(((window_t*)window->inner)->window_handle, HWND_TOP, 0, 0, window->info.width, window->info.height, SWP_NOMOVE | SWP_SHOWWINDOW);
    ERRCHECK_V(ret, *context->result, PX_FAILED_OSCALL);
}

void PxiUpdateHeight(PxContext *context, PxWindow *window, int new_height) {
    window->info.height = new_height;
    WINBOOL ret = SetWindowPos(((window_t*)window->inner)->window_handle, HWND_TOP, 0, 0, window->info.width, window->info.height, SWP_NOMOVE | SWP_SHOWWINDOW);
    ERRCHECK_V(ret, *context->result, PX_FAILED_OSCALL);
}

void PxiUpdateTitle(PxContext *context, PxWindow *window, const char *new_title) {
    window->info.title = realloc(window->info.title, strlen(new_title)+1);
    ERRCHECK_V(window->info.title, *context->result, PX_FAILED_ALLOC);
    strcpy(window->info.title, new_title);

    SetWindowTextA(((window_t*)window->inner)->window_handle, window->info.title);
}