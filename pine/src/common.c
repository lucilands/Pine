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
        case PX_FAILED_OSCALL: return "Failed call to Operating System";

        case PX_INVALID_PARAM: return "Invalid parameter";
        case PX_FAILED_FUNCTION_FETCH: return "Failed to get function from OpenGL";
        case PX_FAILED_OGL_CONTEXT: return "Failed to create OpenGL context";
    }
}

PxResult PxGetContextResult(PxContext *context) {
    return *context->result;
}

void *PxGetWindowParam(PxContext *context, PxWindow *window, enum PxWindowParam param) {
    switch (param) {
        case PX_PARAM_SHOULD_CLOSE: return (void*)&window->should_close;

        case PX_PARAM_SIZE: return (void*)(int[2]){window->info.width, window->info.height};
        case PX_PARAM_POSITION: return (void*)(int[2]){window->info.x, window->info.y};
        case PX_PARAM_RECT: return (void*)(int[4]){window->info.width, window->info.height, window->info.x, window->info.y};

        case PX_PARAM_TITLE: return (void*)window->info.title;

        case PX_PARAM_CONTEXT: return (void*)window->ctx;

        default:
            *context->result = PX_INVALID_PARAM;
            return NULL;
    }
}

void PxSetWindowParam(PxContext *context, PxWindow *window, enum PxWindowParam param, void *value) {
    switch (param) {
        case PX_PARAM_SHOULD_CLOSE: window->should_close = ptrtoi(value); return;

        case PX_PARAM_SIZE: PxiUpdateSize(context, window, value); return;
        case PX_PARAM_POSITION: PxiUpdatePosition(context, window, value); return;

        case PX_PARAM_RECT: PxiUpdateRect(context, window, value); return;

        case PX_PARAM_TITLE: PxiUpdateTitle(context, window, (char*)value); return;

        case PX_PARAM_CONTEXT: *context->result = PX_INVALID_PARAM; return;

        default:
            *context->result = PX_INVALID_PARAM;
            return;
    }
}


PxiEventStack PxiCreateEventStack(PxContext *context) {
    PxiEventStack ret = {
        PxMalloc(sizeof(PxEvent)),
        0,
        sizeof(PxEvent)
    };
    ERRCHECK_T(ret.data, *context->result, PX_FAILED_ALLOC, PxiEventStack);
    return ret;
}

void PxiPushEventStack(PxContext *context, PxiEventStack *cache, PxEvent ev) {
    if ((cache->len + 1) * sizeof(PxEvent) > cache->bytelen) {
        cache->data = PxRealloc(cache->data, (cache->len + 1) * sizeof(PxEvent));
        ERRCHECK_V(cache->data, *context->result, PX_FAILED_ALLOC);
    }
    cache->data[cache->len++] = ev;
}

PxEvent PxiPopEventStack(PxiEventStack *cache) {
    return cache->data[--cache->len];
}

void *PxGetEventParam(PxEvent event) {
    switch (event.type)
    {
    case PX_EVENT_CLOSE: return NULL;
    case PX_EVENT_RESIZE: return NULL;
    case PX_EVENT_WINDOW_MOVE: return NULL;
    
    default:
        return NULL;
    }
}
