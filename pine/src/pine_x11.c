#include <pine/pine.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrender.h>

#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <dlfcn.h>

#ifdef PX_INCLUDE_STDLIB
#include <stdlib.h>
#endif // PX_INCLUDE_STDLIB

#include <GL/gl.h>
#include <GL/glx.h>


#include <string.h>

#include "common.h"

typedef struct {
    Display *disp;
    unsigned int x11_fd;

    XVisualInfo *vi;
    Window root;

    GLXFBConfig fbconfig;
} context_t;

typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

typedef struct {
    Window win;
    Atom close_event;
    XEvent ev;
    XSetWindowAttributes swa;
    Colormap cmap;
    
    struct timeval tv;
    fd_set in_fds;
    
    GLXContext gl_loadctx;
    glXCreateContextAttribsARBProc gl_loadproc;

    GLXContext gl_ctx;
} window_t;

GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
static int VisData[] = {
GLX_RENDER_TYPE, GLX_RGBA_BIT,
    GLX_DOUBLEBUFFER, True,
    GLX_RED_SIZE, 1,
    GLX_GREEN_SIZE, 1,
    GLX_BLUE_SIZE, 1,
    GLX_DEPTH_SIZE, 1,
    None
};
static const char *LIBGLNAMES[] = {"libGL.so.1", "libGL.so"};
static void *libGL;
typedef void* (APIENTRYP PFNGLXGETPROCADDRESSPROC_PRIVATE)(const char*);
static PFNGLXGETPROCADDRESSPROC_PRIVATE GetProcAddressPtr;

#define WHY_THE_FUCK_DOES_X11_GIVE_ME_A_ZERO_VALUE_BACK_IF_IT_CANT_TAKE_ONE_ASSERT_MACRO xce.width > 0 ? xce.width : 1, xce.height > 0 ? xce.height : 1


PxContext *PxCreateContext(PxResult *res) {
    PxContext *ret = PxMalloc(sizeof(struct PxContext_internal));
    ERRCHECK_N(ret, *res, PX_FAILED_ALLOC);
    ret->result = res;

    ret->inner = PxMalloc(sizeof(context_t));
    ERRCHECK_N(ret->inner, *res, PX_FAILED_ALLOC);

    context_t *ctx = (context_t*)ret->inner;
    
    ctx->disp = XOpenDisplay(NULL);
    ctx->root = DefaultRootWindow(ctx->disp);
    ERRCHECK_N(ctx->disp, *res, PX_FAILED_OSCALL);

    ctx->x11_fd = ConnectionNumber(ctx->disp);

    //ctx->vi = glXChooseVisual(ctx->disp, DefaultScreen(ctx->disp), att);
    
    
    int numfbconfigs = 0;
    GLXFBConfig *fbconfigs = glXChooseFBConfig(ctx->disp, DefaultScreen(ctx->disp), VisData, &numfbconfigs);
    ERRCHECK_N(numfbconfigs > 0, *res, PX_FAILED_OSCALL);
	ctx->fbconfig = fbconfigs[0];
    ERRCHECK_N(ctx->fbconfig, *res, PX_FAILED_OSCALL);

    ctx->vi = glXGetVisualFromFBConfig(ctx->disp, ctx->fbconfig);


    //ctx->fbconfig = glXChooseFBConfig(ctx->disp, DefaultScreenOfDisplay(ctx->disp), ctx->vi, int *nitems)

    return ret;
}

PxWindow *PxCreateWindow(PxContext *context, const PxWindowInfo info, PxWindow *parent) {
    PxWindow *ret = PxMalloc(sizeof(struct PxWindow_internal));
    ERRCHECK_N(ret, *context->result, PX_FAILED_ALLOC);
    PxMemset(ret, 0, sizeof(struct PxWindow_internal));

    ret->inner = PxMalloc(sizeof(window_t));
    ERRCHECK_N(ret, *context->result, PX_FAILED_ALLOC);
    PxMemset(ret->inner, 0, sizeof(window_t));

    ret->ctx = context;

    window_t *win = (window_t*)ret->inner;
    context_t *ctx = (context_t*)context->inner;
   
    win->cmap = XCreateColormap(ctx->disp, ctx->root, ctx->vi->visual, AllocNone);
    
    win->swa = (XSetWindowAttributes) {0};
    win->swa.colormap = win->cmap;
    win->swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask;

    
    win->win = XCreateWindow(
        ctx->disp,
        parent ? ((window_t*)parent->inner)->win : ctx->root,
        info.x, info.y,
        info.width, info.height,
        CopyFromParent,
        ctx->vi->depth,
        InputOutput,
        ctx->vi->visual,
        CWColormap | CWEventMask,
        &win->swa
    );

    ERRCHECK_N(win->win, *context->result, PX_FAILED_OSCALL);

    ret->should_close = PX_FALSE;

    XMapWindow(ctx->disp, win->win);
    XStoreName(ctx->disp, win->win, info.title);

    XFlush(ctx->disp);

    win->close_event = XInternAtom(ctx->disp, "WM_DELETE_WINDOW", PX_FALSE);
    XSetWMProtocols(ctx->disp, win->win, &win->close_event, 1);

    XSelectInput(ctx->disp, win->win, ExposureMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask);
    
    win->gl_loadctx = glXCreateContext(ctx->disp, ctx->vi, NULL, PX_TRUE);
    ERRCHECK_N(win->gl_loadctx, *context->result, PX_FAILED_OGL_CONTEXT);
    glXMakeCurrent(ctx->disp, win->win, win->gl_loadctx);

    win->gl_loadproc = (glXCreateContextAttribsARBProc)glXGetProcAddressARB((const GLubyte *)"glXCreateContextAttribsARB");
    ERRCHECK_N(win->gl_loadproc, *context->result, PX_FAILED_FUNCTION_FETCH);
    
    glXMakeCurrent(NULL, None, 0);

    XMoveWindow(ctx->disp, win->win, info.x, info.y);
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

    int event_recv = select(ctx->x11_fd + 1, &win->in_fds, NULL, NULL, &win->tv);

    XConfigureEvent xce = win->ev.xconfigure;

    int pending = XPending(ctx->disp);
    if (pending && event_recv) {
        XNextEvent(ctx->disp, &win->ev);
        
        switch (win->ev.type) {            
            case KeyPress:
                (*ev).type = PX_EVENT_KEYDOWN;
                (*ev).keycode = PxiTransmogKeycode(XLookupKeysym(&win->ev.xkey, 0));
                break;

            case KeyRelease:
                (*ev).type = PX_EVENT_KEYUP;
                (*ev).keycode = PxiTransmogKeycode(XLookupKeysym(&win->ev.xkey, 0));
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
    return pending;
}

void PxiUpdateTitle(PxContext *context, PxWindow *window, const char *new_title) {
    window->info.title = realloc(window->info.title, strlen(new_title) + 1);
    ERRCHECK_V(window->info.title, *context->result, PX_FAILED_ALLOC);
    strcpy(window->info.title, new_title);

    XStoreName(((context_t*)context->inner)->disp, ((window_t*)window->inner)->win, window->info.title);
}

void PxiUpdatePosition(PxContext *context, PxWindow *window, int *new_position) {
    (void)context;
    window->info.x = new_position[0];
    window->info.y = new_position[1];

    //XMoveWindow(((context_t*)context->inner)->disp, ((window_t*)window->inner)->win, window->info.x, window->info.y);
}

void PxiUpdateSize(PxContext *context, PxWindow *window, int *new_size) {
    (void)context;
    window->info.width = new_size[0];
    window->info.height = new_size[1];

    //XResizeWindow(((context_t*)context->inner)->disp, ((window_t*)window->inner)->win, window->info.width, window->info.height);
}

void PxiUpdateRect(PxContext *context, PxWindow *window, int *new_rect) {
    (void)context;
    window->info.width = new_rect[0];
    window->info.height = new_rect[1];
    window->info.x = new_rect[2];
    window->info.y = new_rect[3];

    //XMoveResizeWindow(((context_t*)context->inner)->disp, ((window_t*)window->inner)->win, window->info.x, window->info.y, window->info.width, window->info.height);
}

void* PxiGLLoadproc(const char *name) {
    if (!libGL) return NULL;
    void *res = NULL;
    if (GetProcAddressPtr) {
        res = GetProcAddressPtr(name);
    }
    if (!res) {
        res = dlsym(libGL, name);
    }

    return res;
}

void PxDestroyWindow(PxWindow *window) {
    window_t *win = (window_t*)window->inner;
    context_t *ctx = (context_t*)window->ctx->inner;
    XUnmapWindow(ctx->disp, win->win);
    XDestroyWindow(ctx->disp, win->win);

    glXDestroyContext(((context_t*)window->ctx->inner)->disp, ((window_t*)window->inner)->gl_loadctx);
    if (((window_t*)window->inner)->gl_ctx) glXDestroyContext(((context_t*)window->ctx->inner)->disp, ((window_t*)window->inner)->gl_ctx);

    PxFree(window->inner);
    PxFree(window->info.title);
    //PxFree(window->ecache.data);
    PxFree(window);
}

void PxLoadOpenGL(PxContext *context, PxWindow *window, unsigned short version_major, unsigned short version_minor) {
    for(unsigned int index = 0; index < (sizeof(LIBGLNAMES) / sizeof(LIBGLNAMES[0])); index++) {
        libGL = dlopen(LIBGLNAMES[index], RTLD_NOW | RTLD_GLOBAL);
    }
    ERRCHECK_V(libGL, *context->result, PX_FAILED_DLOPEN)
    GetProcAddressPtr = (PFNGLXGETPROCADDRESSPROC_PRIVATE)dlsym(libGL, "glXGetProcAddressARB");

    int contextAttribs[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, version_major,
        GLX_CONTEXT_MINOR_VERSION_ARB, version_minor,
        GLX_CONTEXT_FLAGS_ARB, 0,
        GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
        //WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0
    };

    ((window_t*)window->inner)->gl_ctx = ((window_t*)window->inner)->gl_loadproc(((context_t*)context->inner)->disp, ((context_t*)context->inner)->fbconfig, 0, PX_TRUE, contextAttribs);
    ERRCHECK_V(((window_t*)window->inner)->gl_ctx, *context->result, PX_FAILED_OGL_CONTEXT)

    glXMakeCurrent(((context_t*)context->inner)->disp, ((window_t*)window->inner)->win, ((window_t*)window->inner)->gl_ctx);
    gladLoadGLLoader(PxiGLLoadproc);
}

void PxDestroyContext(PxContext *context) {
    XCloseDisplay(((context_t*)context->inner)->disp);
    PxFree(context->inner);
    PxFree(context);
}

void PxSwapBuffers(PxWindow *window) {
    glXSwapBuffers(((context_t*)window->ctx->inner)->disp, ((window_t*)window->inner)->win);
}

int PxiTransmogKeycode(int keycode) {
    switch (keycode) {
        case XK_space: return PX_KEY_SPACE;
        case XK_0: return PX_KEY_0;
        case XK_1: return PX_KEY_1;
        case XK_2: return PX_KEY_2;
        case XK_3: return PX_KEY_3;
        case XK_4: return PX_KEY_4;
        case XK_5: return PX_KEY_5;
        case XK_6: return PX_KEY_6;
        case XK_7: return PX_KEY_7;
        case XK_8: return PX_KEY_8;
        case XK_9: return PX_KEY_9;

        case XK_KP_0: return PX_KEY_NP_0;
        case XK_KP_1: return PX_KEY_NP_1;
        case XK_KP_2: return PX_KEY_NP_2;
        case XK_KP_3: return PX_KEY_NP_3;
        case XK_KP_4: return PX_KEY_NP_4;
        case XK_KP_5: return PX_KEY_NP_5;
        case XK_KP_6: return PX_KEY_NP_6;
        case XK_KP_7: return PX_KEY_NP_7;
        case XK_KP_8: return PX_KEY_NP_8;
        case XK_KP_9: return PX_KEY_NP_9;


        case XK_a: case XK_A: return PX_KEY_A;
        case XK_b: case XK_B: return PX_KEY_B;
        case XK_c: case XK_C: return PX_KEY_C;
        case XK_d: case XK_D: return PX_KEY_D;
        case XK_e: case XK_E: return PX_KEY_E;
        case XK_f: case XK_F: return PX_KEY_F;
        case XK_g: case XK_G: return PX_KEY_G;
        case XK_h: case XK_H: return PX_KEY_H;
        case XK_i: case XK_I: return PX_KEY_I;
        case XK_j: case XK_J: return PX_KEY_J;
        case XK_k: case XK_K: return PX_KEY_K;
        case XK_l: case XK_L: return PX_KEY_L;
        case XK_m: case XK_M: return PX_KEY_M;
        case XK_n: case XK_N: return PX_KEY_N;
        case XK_o: case XK_O: return PX_KEY_O;
        case XK_p: case XK_P: return PX_KEY_P;
        case XK_q: case XK_Q: return PX_KEY_Q;
        case XK_r: case XK_R: return PX_KEY_R;
        case XK_s: case XK_S: return PX_KEY_S;
        case XK_t: case XK_T: return PX_KEY_T;
        case XK_u: case XK_U: return PX_KEY_U;
        case XK_v: case XK_V: return PX_KEY_V;
        case XK_w: case XK_W: return PX_KEY_W;
        case XK_x: case XK_X: return PX_KEY_X;
        case XK_y: case XK_Y: return PX_KEY_Y;
        case XK_z: case XK_Z: return PX_KEY_Z;

        case XK_Home: return PX_KEY_HOME;
        case XK_End: return PX_KEY_END;
        case XK_Left: return PX_KEY_LEFT;
        case XK_Right: return PX_KEY_RIGHT;
        case XK_Up: return PX_KEY_UP;
        case XK_Down: return PX_KEY_DOWN;
        case XK_Page_Up: return PX_KEY_PAGE_UP;
        case XK_Page_Down: return PX_KEY_PAGE_DOWN;

        default: return PX_KEY_UNKNOWN;
    }
}
