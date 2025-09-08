#ifndef _COMMON_H
#define _COMMON_H

#include <pine.h>

typedef struct {
    PxEvent *data;
    PxUint16 len;
    PxUint32 bytelen;
} PxiEventStack;

struct PxWindow_internal {
    void *inner;
    PxUint8 should_close;
    PxWindowInfo info;
    PxiEventStack ecache;
    PxContext *ctx;
};

struct PxContext_internal {
    void *inner;
    PxResult *result;
};


// Internal utility functions (Pxi = Pine Internal)

#define ERRCHECK_N(var_to_check, res, errcode) if (!(var_to_check)) {(res) = (errcode); return NULL;}
#define ERRCHECK_V(var_to_check, res, errcode) if (!(var_to_check)) {(res) = (errcode); return;}
#define ERRCHECK_T(var_to_check, res, errcode, type) if (!(var_to_check)) {(res) = (errcode); return (type){0};}

void PxiUpdateTitle(PxContext *context, PxWindow *window, const char *new_title);

void PxiUpdatePosition(PxContext *context, PxWindow *window, int *new_position);
void PxiUpdateSize(PxContext *context, PxWindow *window, int *new_size);
void PxiUpdateRect(PxContext *context, PxWindow *window, int *new_rect);

int PxiTransmogKeycode(int keycode);

PxiEventStack PxiCreateEventStack(PxContext *context);
void PxiPushEventStack(PxContext *context, PxiEventStack *cache, PxEvent ev);
PxEvent PxiPopEventStack(PxiEventStack *cache);

#endif //_COMMON_H
