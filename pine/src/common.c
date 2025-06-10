#include <pine.h>
#include "common.h"

#ifdef PX_INCLUDE_STDLIB
#include <stdlib.h>
#endif // PX_INCLUDE_STDLIB

int ptrtoi(int *a) {return *a;}

char *PxErrorToString(PxResult res) {
    switch (res) {
        default: return "Invalid error code";
        
        case PX_SUCCESS: return "No error";
        case PX_FAILED_ALLOC: return "Failed to allocate memory";
        case PX_FAILED_WINDOW_CREATION: return "Failed to create window";
        case PX_FAILED_HANDLE_FETCH: return "Failed to fetch handle";
        case PX_FAILED_STRING: return "Failed string operation";

        case PX_WRONG_PARAM: return "Invalid parameter";
    }
}

PxResult PxGetContextResult(PxContext *context) {
    return *context->result;
}

void PxDestroyContext(PxContext *context) {
    PxFree(context->inner);
    PxFree(context);
}

void *PxGetWindowParam(PxContext *context, PxWindow *window, enum PxWindowParam param) {
    switch (param) {
        case PX_PARAM_SHOULD_CLOSE: return (void*)&window->should_close;

        case PX_PARAM_WIDTH: return (void*)&window->info.width;
        case PX_PARAM_HEIGHT: return (void*)&window->info.height;

        case PX_PARAM_X: return (void*)&window->info.x;
        case PX_PARAM_Y: return (void*)&window->info.y;

        case PX_PARAM_TITLE: return (void*)&window->info.title;

        default:
            *context->result = PX_WRONG_PARAM;
            return NULL;
    }
}

void PxSetWindowParam(PxContext *context, PxWindow *window, enum PxWindowParam param, void *value) {
    switch (param) {
        case PX_PARAM_SHOULD_CLOSE: window->should_close = ptrtoi(value); return;

        case PX_PARAM_WIDTH: PxiUpdateWidth(context, window, ptrtoi(value)); return;
        case PX_PARAM_HEIGHT: PxiUpdateHeight(context, window, ptrtoi(value)); return;

        case PX_PARAM_X: PxiUpdateX(context, window, ptrtoi(value)); return;
        case PX_PARAM_Y: PxiUpdateY(context, window, ptrtoi(value)); return;

        case PX_PARAM_TITLE: PxiUpdateTitle(context, window, (char*)value); return;

        default:
            *context->result = PX_WRONG_PARAM;
            return;
    }
}

void PxDestroyWindow(PxWindow *window) {
    PxFree(window->inner);
    PxFree(window->info.title);
    PxFree(window);
}