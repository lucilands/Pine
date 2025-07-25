#include <pine.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>

#ifdef PX_INCLUDE_STDLIB
#include <stdlib.h>
#endif // PX_INCLUDE_STDLIB

#include <string.h>

#include "common.h"

typedef struct {
    Display *disp;
    unsigned int x11_fd;
} context_t;

typedef struct {
    Window win;
    Atom close_event;
    XEvent ev;

    struct timeval tv;
    fd_set in_fds;
} window_t;

#define WHY_THE_FUCK_DOES_X11_GIVE_ME_A_ZERO_VALUE_BACK_IF_IT_CANT_TAKE_ONE_ASSERT_MACRO xce.width > 0 ? xce.width : 1, xce.height > 0 ? xce.height : 1

PxContext *PxCreateContext(PxResult *res) {
    PxContext *ret = PxMalloc(sizeof(struct PxContext_internal));
    ERRCHECK_N(ret, *res, PX_FAILED_ALLOC);
    ret->result = res;

    ret->inner = PxMalloc(sizeof(context_t));
    ERRCHECK_N(ret->inner, *res, PX_FAILED_ALLOC);

    context_t *ctx = (context_t*)ret->inner;
    
    ctx->disp = XOpenDisplay(NULL);
    ERRCHECK_N(ctx->disp, *res, PX_FAILED_OSCALL);

    ctx->x11_fd = ConnectionNumber(ctx->disp);

    return ret;
}

PxWindow *PxCreateWindow(PxContext *context, const PxWindowInfo info, PxWindow *parent) {
    PxWindow *ret = PxMalloc(sizeof(struct PxWindow_internal));
    ERRCHECK_N(ret, *context->result, PX_FAILED_ALLOC);

    ret->inner = PxMalloc(sizeof(window_t));
    ERRCHECK_N(ret, *context->result, PX_FAILED_ALLOC);

    ret->ctx = context;

    window_t *win = (window_t*)ret->inner;
    context_t *ctx = (context_t*)context->inner;

    win->win = XCreateSimpleWindow(
        ctx->disp,
        win->win ? ((window_t*)parent->inner)->win : RootWindow(ctx->disp, 0),
        info.x, info.y,
        info.width, info.height,
        0,
        0x0,
        0x0
    );
    ERRCHECK_N(win->win, *context->result, PX_FAILED_OSCALL);
    XStoreName(ctx->disp, win->win, info.title);

    XFlush(ctx->disp);

    ret->should_close = PX_FALSE;

    XGrabKeyboard(ctx->disp, win->win, 0, GrabModeAsync, GrabModeAsync, CurrentTime);
    XSelectInput(ctx->disp, win->win, KeyPressMask | KeyReleaseMask | ExposureMask | StructureNotifyMask);

    XMapWindow(ctx->disp, win->win);

    win->close_event = XInternAtom(ctx->disp, "WM_DELETE_WINDOW", PX_FALSE);
    XSetWMProtocols(ctx->disp, win->win, &win->close_event, 1);
    return ret;
}


int PxPollEvents(PxWindow *window, PxEvent *ev) {
    window_t *win = (window_t*)window->inner;
    context_t *ctx = (context_t*)window->ctx->inner;
    ev->type = __PX_EVENT_NONE__;

    FD_ZERO(&win->in_fds);
    FD_SET(ctx->x11_fd, &win->in_fds);

    win->tv.tv_usec = 10;
    win->tv.tv_sec = 0;

    int num_ready_fds = select(ctx->x11_fd + 1, &win->in_fds, NULL, NULL, &win->tv);

    XConfigureEvent xce = win->ev.xconfigure;

    if (num_ready_fds > 0) {
        XNextEvent(ctx->disp, &win->ev);
        
        switch (win->ev.type) {            
            case KeyPress:
                (*ev).type = PX_EVENT_KEYDOWN;
                (*ev).keycode = XLookupKeysym(&win->ev.xkey, 0);
                break;

            case KeyRelease:
                (*ev).type = PX_EVENT_KEYUP;
                (*ev).keycode = XLookupKeysym(&win->ev.xkey, 0);
                break;

            case ConfigureNotify:
                if (xce.width != window->info.width || xce.height != window->info.height) {
                    (*ev).type = PX_EVENT_RESIZE;
                    PxiUpdateSize(window->ctx, window, (int[2]){WHY_THE_FUCK_DOES_X11_GIVE_ME_A_ZERO_VALUE_BACK_IF_IT_CANT_TAKE_ONE_ASSERT_MACRO});
                    break;
                }
                else if (xce.x != window->info.x || xce.y != window->info.y) {
                    (*ev).type = PX_EVENT_WINDOW_MOVE;
                    PxiUpdatePosition(window->ctx, window, (int[2]){xce.x, xce.y});
                    break;
                }
                break;
            
            case ClientMessage:
                if ((Atom)win->ev.xclient.data.l[0] == win->close_event) {
                    (*ev).type = PX_EVENT_CLOSE;
                }
                break;


            default: break; 
        }
    }
    return num_ready_fds > 0;
}

void PxiUpdateTitle(PxContext *context, PxWindow *window, const char *new_title) {
    window->info.title = realloc(window->info.title, strlen(new_title) + 1);
    ERRCHECK_V(window->info.title, *context->result, PX_FAILED_ALLOC);
    strcpy(window->info.title, new_title);

    XStoreName(((context_t*)context->inner)->disp, ((window_t*)window->inner)->win, window->info.title);
}

void PxiUpdatePosition(PxContext *context, PxWindow *window, int *new_position) {
    window->info.x = new_position[0];
    window->info.y = new_position[1];

    XMoveWindow(((context_t*)context->inner)->disp, ((window_t*)window->inner)->win, window->info.x, window->info.y);
}

void PxiUpdateSize(PxContext *context, PxWindow *window, int *new_size) {
    window->info.width = new_size[0];
    window->info.height = new_size[1];

    XResizeWindow(((context_t*)context->inner)->disp, ((window_t*)window->inner)->win, window->info.width, window->info.height);
}

void PxiUpdateRect(PxContext *context, PxWindow *window, int *new_rect) {
    window->info.width = new_rect[0];
    window->info.height = new_rect[1];
    window->info.x = new_rect[2];
    window->info.y = new_rect[3];

    XMoveResizeWindow(((context_t*)context->inner)->disp, ((window_t*)window->inner)->win, window->info.x, window->info.y, window->info.width, window->info.height);
}

void PxDestroyWindow(PxWindow *window) {
    window_t *win = (window_t*)window->inner;
    context_t *ctx = (context_t*)window->ctx->inner;
    XUnmapWindow(ctx->disp, win->win);
    XDestroyWindow(ctx->disp, win->win);
    XCloseDisplay(ctx->disp);

    PxFree(window->inner);
    PxFree(window->info.title);
    PxFree(window->ecache.data);
    PxFree(window);
}

void PxDestroyContext(PxContext *context) {
    PxFree(context->inner);
    PxFree(context);
}
