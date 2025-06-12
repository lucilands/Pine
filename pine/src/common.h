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


void PxiUpdatePosition(PxContext *context, PxWindow *window, int *new_position);
void PxiUpdateSize(PxContext *context, PxWindow *window, int *new_size);
void PxiUpdateRect(PxContext *context, PxWindow *window, int *new_rect);

//void PxiUpdateX(PxContext *context, PxWindow *window, int new_x);
//void PxiUpdateY(PxContext *context, PxWindow *window, int new_y);
//void PxiUpdateWidth(PxContext *context, PxWindow *window, int new_width);
//void PxiUpdateHeight(PxContext *context, PxWindow *window, int new_height);

#endif //_COMMON_H