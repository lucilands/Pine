#ifndef _PINE_H
#define _PINE_H

#include "pxutils.h"

typedef struct PxContext_internal PxContext;
typedef struct PxWindow_internal PxWindow;

typedef struct {
    PxInt32 width, height;
    PxInt32 x, y;
    char *title;
} PxWindowInfo;

typedef struct {
    const PxInt32 width, height;
    const PxUint8 amount;
} PxDisplayInfo;

enum PxWindowParam {
    PX_PARAM_SHOULD_CLOSE = 0,

    PX_PARAM_SIZE,
    PX_PARAM_POSITION,
    PX_PARAM_RECT,

    PX_PARAM_TITLE,
};

PxDisplayInfo PxGetDisplay(PxContext *context);

PxContext *PxCreateContext(PxResult *res);
PxWindow *PxCreateWindow(PxContext *context, const PxWindowInfo info, PxWindow *parent);

void *PxGetWindowParam(PxContext *context, PxWindow *window, enum PxWindowParam param);
void PxSetWindowParam(PxContext *context, PxWindow *window, enum PxWindowParam param, void *value);

char *PxErrorToString(PxResult res);
PxResult PxGetContextResult(PxContext *context);

void PxPollEvents(PxWindow *window);

void PxDestroyContext(PxContext *context);
void PxDestroyWindow(PxWindow *window);

#endif //_PINE_H