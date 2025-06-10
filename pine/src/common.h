#ifndef _COMMON_H
#define _COMMON_H

#include <pxutils.h>

struct PxWindow_internal {
    void *inner;
    PxUint8 should_close;
    PxWindowInfo info;
};

struct PxContext_internal {
    void *inner;
    PxResult *result;
};

// Internal utility functions

void PxiUpdateTitle(PxContext *context, PxWindow *window, const char *new_title);

// TODO: Merge PxiUpdateX and PxiUpdateY into single function
void PxiUpdateX(PxContext *context, PxWindow *window, int new_x);
void PxiUpdateY(PxContext *context, PxWindow *window, int new_y);

// TODO: Merge PxiUpdateWidth and PxiUpdateHeight into single function
void PxiUpdateWidth(PxContext *context, PxWindow *window, int new_width);
void PxiUpdateHeight(PxContext *context, PxWindow *window, int new_height);
#endif //_COMMON_H