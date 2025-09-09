#ifndef _PINE_H
#define _PINE_H
// TODO: Implement PX_KEYCODE_xx

#include "pxutils.h"
#include "pxkeycodes.h"
#include "pxglad.h"

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

typedef struct {
    int type;
    int keycode;
} PxEvent;

enum PxWindowParam {
    PX_PARAM_SHOULD_CLOSE = 0,

    PX_PARAM_SIZE,
    PX_PARAM_POSITION,
    PX_PARAM_RECT,

    PX_PARAM_TITLE,

    PX_PARAM_CONTEXT,
};

enum PxEventType {
    __PX_EVENT_NONE__ = 0,
    PX_EVENT_CLOSE,
    PX_EVENT_RESIZE,
    PX_EVENT_WINDOW_MOVE,

    PX_EVENT_KEYDOWN,
    PX_EVENT_KEYUP
};

PxDisplayInfo PxGetDisplay(PxContext *context);

PxContext *PxCreateContext(PxResult *res);
PxWindow *PxCreateWindow(PxContext *context, const PxWindowInfo info, PxWindow *parent);

void *PxGetWindowParam(PxContext *context, PxWindow *window, enum PxWindowParam param);
void *PxGetEventParam(PxEvent event);
void PxSetWindowParam(PxContext *context, PxWindow *window, enum PxWindowParam param, void *value);
void PxLoadOpenGL(PxContext *context, PxWindow *window, unsigned short version_major, unsigned short version_minor);

char *PxErrorToString(PxResult res);
PxResult PxGetContextResult(PxContext *context);

int PxPollEvents(PxWindow *window, PxEvent *ev);

void PxCloseWindow(PxContext *context, PxWindow *window);

void PxDestroyContext(PxContext *context);
void PxDestroyWindow(PxWindow *window);

void PxSwapBuffers(PxWindow *window);

#endif //_PINE_H
