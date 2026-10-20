// For poll(), mmap() and friends under -std=c99.
#define _POSIX_C_SOURCE 200809L

#include "pine/pxutils.h"
#include <pine/pine.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <dlfcn.h>
#include <xdg-shell-client-protocol.h>
#include <xkbcommon/xkbcommon.h>
#include <linux/input-event-codes.h>

#include <errno.h>
#include <poll.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#ifdef PX_INCLUDE_STDLIB
#include <stdlib.h>
#endif // PX_INCLUDE_STDLIB

#include "common.h"



typedef struct {
    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_compositor *compositor;
    struct xdg_wm_base *xdg_wm_base;
    EGLDisplay egl_display;

    struct wl_seat *seat;
    struct wl_keyboard *keyboard;
    struct wl_pointer *pointer;
    PxWindow *ptr_focus;
    PxInt32 ptr_x, ptr_y;
    struct xkb_context *xkb_context;
    struct xkb_keymap *xkb_keymap;
    struct xkb_state *xkb_state;
    PxWindow *kb_focus;

    int32_t repeat_rate, repeat_delay;
    PxUint8 repeat_active;
    uint32_t repeat_key;
    int repeat_keycode;
    int64_t repeat_next_ms;
} context_t;


typedef struct {
    struct wl_surface *surf;
    struct xdg_surface *xdg_surf;
    struct xdg_toplevel *xdg_toplevel;
    struct wl_egl_window *egl_window;
    PxUint8 configured;
    PxInt32 pending_width, pending_height;
    EGLSurface egl_surface;
    EGLContext egl_context;
} window_t;

static const char *LIBGLNAMES[] = {"libGL.so.1", "libGL.so"};
static void *libGL;

static int64_t monotonic_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static void stop_key_repeat(context_t *ctx) {
    ctx->repeat_active = PX_FALSE;
}

static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial) {
    (void)data;
    xdg_wm_base_pong(xdg_wm_base, serial);
}
static const struct xdg_wm_base_listener xdg_wm_base_listener = { .ping = xdg_wm_base_ping };

static void push_event(PxWindow *window, int type, int keycode) {
    PxEvent ev = { .type = type, .keycode = keycode };
    PxiPushEventStack(window->ctx, &window->ecache, ev);
}

static PxEvent pop_front_event(PxiEventStack *cache) {
    PxEvent ev = cache->data[0];
    cache->len--;
    memmove(cache->data, cache->data + 1, cache->len * sizeof(PxEvent));
    return ev;
}

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial) {
    PxWindow *window = (PxWindow*)data;
    window_t *win = (window_t*)window->inner;
    xdg_surface_ack_configure(xdg_surface, serial);
    win->configured = PX_TRUE;

    if (win->pending_width <= 0 || win->pending_height <= 0) return;
    if (win->pending_width == window->info.width && win->pending_height == window->info.height) return;
    window->info.width = win->pending_width;
    window->info.height = win->pending_height;
    wl_egl_window_resize(win->egl_window, window->info.width, window->info.height, 0, 0);
    push_event(window, PX_EVENT_RESIZE, 0);
}
static const struct xdg_surface_listener xdg_surface_listener = { .configure = xdg_surface_configure };

static void xdg_toplevel_configure(void *data, struct xdg_toplevel *xdg_toplevel, int32_t width, int32_t height, struct wl_array *states) {
    (void)xdg_toplevel; (void)states;
    window_t *win = (window_t*)((PxWindow*)data)->inner;
    win->pending_width = width;
    win->pending_height = height;
}
static void xdg_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel) {
    (void)xdg_toplevel;
    PxWindow *window = (PxWindow*)data;
    window->should_close = PX_TRUE;
    push_event(window, PX_EVENT_CLOSE, 0);
}
static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_configure,
    .close = xdg_toplevel_close
};

static void keyboard_keymap(void *data, struct wl_keyboard *keyboard, uint32_t format, int32_t fd, uint32_t size) {
    (void)keyboard;
    PxContext *context = (PxContext*)data;
    context_t *ctx = (context_t*)context->inner;

    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) { close(fd); return; }

    char *map = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    ERRCHECK_V(map != MAP_FAILED, *context->result, PX_FAILED_OSCALL);

    struct xkb_keymap *keymap = xkb_keymap_new_from_string(ctx->xkb_context, map, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
    munmap(map, size);
    ERRCHECK_V(keymap, *context->result, PX_FAILED_OSCALL);

    struct xkb_state *state = xkb_state_new(keymap);
    if (!state) { xkb_keymap_unref(keymap); *context->result = PX_FAILED_ALLOC; return; }

    if (ctx->xkb_state) xkb_state_unref(ctx->xkb_state);
    if (ctx->xkb_keymap) xkb_keymap_unref(ctx->xkb_keymap);
    ctx->xkb_keymap = keymap;
    ctx->xkb_state = state;
}

static void keyboard_enter(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface, struct wl_array *keys) {
    (void)keyboard; (void)serial; (void)keys;
    context_t *ctx = (context_t*)((PxContext*)data)->inner;
    ctx->kb_focus = surface ? (PxWindow*)wl_surface_get_user_data(surface) : NULL;
}

static void keyboard_leave(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface) {
    (void)keyboard; (void)serial; (void)surface;
    context_t *ctx = (context_t*)((PxContext*)data)->inner;
    ctx->kb_focus = NULL;
    stop_key_repeat(ctx);
}

static void keyboard_key(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {
    (void)keyboard; (void)serial; (void)time;
    context_t *ctx = (context_t*)((PxContext*)data)->inner;
    if (!ctx->kb_focus || !ctx->xkb_state) return;

    xkb_keycode_t keycode = key + 8;
    xkb_layout_index_t layout = xkb_state_key_get_layout(ctx->xkb_state, keycode);
    const xkb_keysym_t *syms = NULL;
    int nsyms = xkb_keymap_key_get_syms_by_level(ctx->xkb_keymap, keycode, layout, 0, &syms);
    xkb_keysym_t sym = nsyms > 0 ? syms[0] : xkb_state_key_get_one_sym(ctx->xkb_state, keycode);

    int type = state == WL_KEYBOARD_KEY_STATE_PRESSED ? PX_EVENT_KEYDOWN : PX_EVENT_KEYUP;
    int pine_keycode = PxiTransmogKeycode((int)sym);
    push_event(ctx->kb_focus, type, pine_keycode);

    if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        if (ctx->repeat_rate > 0 && xkb_keymap_key_repeats(ctx->xkb_keymap, keycode)) {
            ctx->repeat_active = PX_TRUE;
            ctx->repeat_key = key;
            ctx->repeat_keycode = pine_keycode;
            ctx->repeat_next_ms = monotonic_ms() + ctx->repeat_delay;
        } else {
            stop_key_repeat(ctx);
        }
    } else if (ctx->repeat_active && ctx->repeat_key == key) {
        stop_key_repeat(ctx);
    }
}

static void dispatch_key_repeat(context_t *ctx) {
    if (!ctx->repeat_active || !ctx->kb_focus || ctx->repeat_rate <= 0) return;

    int64_t now = monotonic_ms();
    if (now < ctx->repeat_next_ms) return;

    push_event(ctx->kb_focus, PX_EVENT_KEYDOWN, ctx->repeat_keycode);
    ctx->repeat_next_ms = now + 1000 / ctx->repeat_rate;
}

static void keyboard_modifiers(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t depressed, uint32_t latched, uint32_t locked, uint32_t group) {
    (void)keyboard; (void)serial;
    context_t *ctx = (context_t*)((PxContext*)data)->inner;
    if (!ctx->xkb_state) return;
    xkb_state_update_mask(ctx->xkb_state, depressed, latched, locked, 0, 0, group);
}

static void keyboard_repeat_info(void *data, struct wl_keyboard *keyboard, int32_t rate, int32_t delay) {
    (void)keyboard;
    context_t *ctx = (context_t*)((PxContext*)data)->inner;
    ctx->repeat_rate = rate;
    ctx->repeat_delay = delay;
    if (rate <= 0) stop_key_repeat(ctx);
}

static const struct wl_keyboard_listener keyboard_listener = {
    .keymap = keyboard_keymap,
    .enter = keyboard_enter,
    .leave = keyboard_leave,
    .key = keyboard_key,
    .modifiers = keyboard_modifiers,
    .repeat_info = keyboard_repeat_info
};

static void pointer_enter(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t sx, wl_fixed_t sy) {
    (void)pointer; (void)serial;
    context_t *ctx = (context_t*)((PxContext*)data)->inner;
    ctx->ptr_focus = surface ? (PxWindow*)wl_surface_get_user_data(surface) : NULL;
    ctx->ptr_x = wl_fixed_to_int(sx);
    ctx->ptr_y = wl_fixed_to_int(sy);
}

static void push_mouse_event(context_t *ctx, int type, int button) {
    PxEvent ev = { .type = type, .keycode = button, .mouse_x = ctx->ptr_x, .mouse_y = ctx->ptr_y };
    PxiPushEventStack(ctx->ptr_focus->ctx, &ctx->ptr_focus->ecache, ev);
}

static void pointer_leave(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface) {
    (void)pointer; (void)serial; (void)surface;
    ((context_t*)((PxContext*)data)->inner)->ptr_focus = NULL;
}

static void pointer_motion(void *data, struct wl_pointer *pointer, uint32_t time, wl_fixed_t sx, wl_fixed_t sy) {
    (void)pointer; (void)time;
    context_t *ctx = (context_t*)((PxContext*)data)->inner;
    ctx->ptr_x = wl_fixed_to_int(sx);
    ctx->ptr_y = wl_fixed_to_int(sy);
    if (!ctx->ptr_focus) return;
    push_mouse_event(ctx, PX_EVENT_MOUSE_MOVE, 0);
}

static int wl_button_to_pine(uint32_t button) {
    switch (button) {
        case BTN_LEFT: return PX_MOUSE_LEFT;
        case BTN_RIGHT: return PX_MOUSE_RIGHT;
        case BTN_MIDDLE: return PX_MOUSE_MIDDLE;
        default: return 0;
    }
}

static void pointer_button(void *data, struct wl_pointer *pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state) {
    (void)pointer; (void)serial; (void)time;
    context_t *ctx = (context_t*)((PxContext*)data)->inner;
    int pine_button = wl_button_to_pine(button);
    if (!ctx->ptr_focus || !pine_button) return;

    int type = state == WL_POINTER_BUTTON_STATE_PRESSED ? PX_EVENT_MOUSE_DOWN : PX_EVENT_MOUSE_UP;
    push_mouse_event(ctx, type, pine_button);
}

static void pointer_axis(void *data, struct wl_pointer *pointer, uint32_t time, uint32_t axis, wl_fixed_t value) {
    (void)data; (void)pointer; (void)time; (void)axis; (void)value;
}

static const struct wl_pointer_listener pointer_listener = {
    .enter = pointer_enter,
    .leave = pointer_leave,
    .motion = pointer_motion,
    .button = pointer_button,
    .axis = pointer_axis
};

static void seat_capabilities(void *data, struct wl_seat *seat, uint32_t capabilities) {
    context_t *ctx = (context_t*)((PxContext*)data)->inner;
    int has_keyboard = capabilities & WL_SEAT_CAPABILITY_KEYBOARD;
    int has_pointer = capabilities & WL_SEAT_CAPABILITY_POINTER;

    if (has_pointer && !ctx->pointer) {
        ctx->pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(ctx->pointer, &pointer_listener, data);
    } else if (!has_pointer && ctx->pointer) {
        wl_pointer_release(ctx->pointer);
        ctx->pointer = NULL;
        ctx->ptr_focus = NULL;
    }

    if (has_keyboard && !ctx->keyboard) {
        ctx->keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(ctx->keyboard, &keyboard_listener, data);
    } else if (!has_keyboard && ctx->keyboard) {
        wl_keyboard_release(ctx->keyboard);
        ctx->keyboard = NULL;
        ctx->kb_focus = NULL;
        stop_key_repeat(ctx);
    }
}

static void seat_name(void *data, struct wl_seat *seat, const char *name) {
    (void)data; (void)seat; (void)name;
}

static const struct wl_seat_listener seat_listener = {
    .capabilities = seat_capabilities,
    .name = seat_name
};

static void registry_global(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version) {
    PxContext *context = (PxContext*)data;
    context_t *ctx = (context_t*)context->inner;
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        ctx->compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 4);
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        ctx->xdg_wm_base = wl_registry_bind(registry, id, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(ctx->xdg_wm_base, &xdg_wm_base_listener, NULL);
    } else if (strcmp(interface, wl_seat_interface.name) == 0 && !ctx->seat) {
        ctx->seat = wl_registry_bind(registry, id, &wl_seat_interface, version < 4 ? version : 4);
        wl_seat_add_listener(ctx->seat, &seat_listener, data);
    }
}
static void registry_global_remove(void *data, struct wl_registry *registry, uint32_t id) {
    (void)data; (void)registry; (void)id;
}
static const struct wl_registry_listener registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove
};

PxContext *PxCreateContext(PxResult *res) {
    PxContext *ret = PxMalloc(sizeof(struct PxContext_internal));
    ERRCHECK_N(ret, *res, PX_FAILED_ALLOC);
    ret->result = res;

    ret->inner = PxMalloc(sizeof(context_t));
    ERRCHECK_N(ret->inner, *res, PX_FAILED_ALLOC);
    PxMemset(ret->inner, 0, sizeof(context_t));

    context_t *ctx = (context_t*)ret->inner;
    ctx->egl_display = EGL_NO_DISPLAY;
    ctx->repeat_rate = 25;
    ctx->repeat_delay = 600;
    ctx->xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    ERRCHECK_N(ctx->xkb_context, *res, PX_FAILED_ALLOC);

    ctx->display = wl_display_connect(NULL);
    ERRCHECK_N(ctx->display, *res, PX_FAILED_OSCALL);

    ctx->registry = wl_display_get_registry(ctx->display);
    ERRCHECK_N(ctx->registry, *res, PX_FAILED_OSCALL);

    wl_registry_add_listener(ctx->registry, &registry_listener, (void*)ret);
    wl_display_roundtrip(ctx->display);
    ERRCHECK_N(ctx->compositor, *res, PX_FAILED_OSCALL);
    ERRCHECK_N(ctx->xdg_wm_base, *res, PX_FAILED_OSCALL);
    wl_display_roundtrip(ctx->display);

    return ret;
}

PxWindow *PxCreateWindow(PxContext *context, const PxWindowInfo info, PxWindow *parent) {
    ERRCHECK_N(!parent, *context->result, PX_INVALID_PARAM);
    ERRCHECK_N(info.width > 0 && info.height > 0, *context->result, PX_INVALID_PARAM);

    PxWindow *ret = PxMalloc(sizeof(struct PxWindow_internal));
    ERRCHECK_N(ret, *context->result, PX_FAILED_ALLOC);
    PxMemset(ret, 0, sizeof(struct PxWindow_internal));

    ret->inner = PxMalloc(sizeof(window_t));
    ERRCHECK_N(ret->inner, *context->result, PX_FAILED_ALLOC);
    PxMemset(ret->inner, 0, sizeof(window_t));

    ret->ctx = context;
    ret->info = info;
    ret->info.title = NULL;
    ret->info.x = 0;
    ret->info.y = 0;
    ret->should_close = PX_FALSE;
    if (info.title) {
        ret->info.title = PxMalloc(PxStrlen(info.title) + 1);
        ERRCHECK_N(ret->info.title, *context->result, PX_FAILED_ALLOC);
        strcpy(ret->info.title, info.title);
    }

    window_t *win = (window_t*)ret->inner;
    context_t *ctx = (context_t*)context->inner;
    win->egl_surface = EGL_NO_SURFACE;
    win->egl_context = EGL_NO_CONTEXT;

    ret->ecache = PxiCreateEventStack(context);
    ERRCHECK_N(ret->ecache.data, *context->result, PX_FAILED_ALLOC);

    win->surf = wl_compositor_create_surface(ctx->compositor);
    ERRCHECK_N(win->surf, *context->result, PX_FAILED_WINDOW_CREATION);
    wl_surface_set_user_data(win->surf, ret);

    win->xdg_surf = xdg_wm_base_get_xdg_surface(ctx->xdg_wm_base, win->surf);
    ERRCHECK_N(win->xdg_surf, *context->result, PX_FAILED_WINDOW_CREATION);
    xdg_surface_add_listener(win->xdg_surf, &xdg_surface_listener, (void*)ret);

    win->xdg_toplevel = xdg_surface_get_toplevel(win->xdg_surf);
    ERRCHECK_N(win->xdg_toplevel, *context->result, PX_FAILED_WINDOW_CREATION);
    xdg_toplevel_add_listener(win->xdg_toplevel, &xdg_toplevel_listener, (void*)ret);
    if (ret->info.title) xdg_toplevel_set_title(win->xdg_toplevel, ret->info.title);

    win->egl_window = wl_egl_window_create(win->surf, info.width, info.height);
    ERRCHECK_N(win->egl_window, *context->result, PX_FAILED_WINDOW_CREATION);

    wl_surface_commit(win->surf);
    while (!win->configured) {
        ERRCHECK_N(wl_display_dispatch(ctx->display) != -1, *context->result, PX_FAILED_OSCALL);
    }

    return ret;
}

static int pump_display(struct wl_display *display) {
    while (wl_display_prepare_read(display) != 0) {
        if (wl_display_dispatch_pending(display) == -1) return -1;
    }
    if (wl_display_flush(display) == -1 && errno != EAGAIN) {
        wl_display_cancel_read(display);
        return -1;
    }

    struct pollfd pfd = { .fd = wl_display_get_fd(display), .events = POLLIN };
    if (poll(&pfd, 1, 0) > 0 && (pfd.revents & POLLIN)) {
        if (wl_display_read_events(display) == -1) return -1;
    } else {
        wl_display_cancel_read(display);
    }
    return wl_display_dispatch_pending(display);
}

int PxPollEvents(PxWindow *window, PxEvent *ev) {
    context_t *ctx = (context_t*)window->ctx->inner;
    ev->type = __PX_EVENT_NONE__;
    ev->keycode = 0;

    if (!window->ecache.len && pump_display(ctx->display) == -1) {
        *window->ctx->result = PX_FAILED_OSCALL;
        if (window->should_close) return 0;
        window->should_close = PX_TRUE;
        ev->type = PX_EVENT_CLOSE;
        return 1;
    }

    if (!window->ecache.len) dispatch_key_repeat(ctx);
    if (!window->ecache.len) return 0;
    *ev = pop_front_event(&window->ecache);
    return 1;
}

void PxiUpdateTitle(PxContext *context, PxWindow *window, const char *new_title) {
    ERRCHECK_V(new_title, *context->result, PX_INVALID_PARAM);
    char *title = PxRealloc(window->info.title, PxStrlen(new_title) + 1);
    ERRCHECK_V(title, *context->result, PX_FAILED_ALLOC);
    strcpy(title, new_title);
    window->info.title = title;

    xdg_toplevel_set_title(((window_t*)window->inner)->xdg_toplevel, window->info.title);
    wl_display_flush(((context_t*)context->inner)->display);
}

void PxiUpdatePosition(PxContext *context, PxWindow *window, int *new_position) {
    (void)window; (void)new_position;
    *context->result = PX_UNSUPPORTED;
}

void PxiUpdateSize(PxContext *context, PxWindow *window, int *new_size) {
    ERRCHECK_V(new_size[0] > 0 && new_size[1] > 0, *context->result, PX_INVALID_PARAM);
    window->info.width = new_size[0];
    window->info.height = new_size[1];

    wl_egl_window_resize(((window_t*)window->inner)->egl_window, window->info.width, window->info.height, 0, 0);
}

void PxiUpdateRect(PxContext *context, PxWindow *window, int *new_rect) {
    PxiUpdateSize(context, window, new_rect);
    if (*context->result == PX_SUCCESS) PxiUpdatePosition(context, window, new_rect + 2);
}

void *PxiGLLoadproc(const char *name) {
    void *res = (void*)eglGetProcAddress(name);
    if (!res && libGL) res = dlsym(libGL, name);
    return res;
}

void PxLoadOpenGL(PxContext *context, PxWindow *window, unsigned short version_major, unsigned short version_minor) {
    context_t *ctx = (context_t*)context->inner;
    window_t *win = (window_t*)window->inner;
    ERRCHECK_V(win->egl_context == EGL_NO_CONTEXT, *context->result, PX_INVALID_PARAM);

    if (ctx->egl_display == EGL_NO_DISPLAY) {
        ctx->egl_display = eglGetPlatformDisplay(EGL_PLATFORM_WAYLAND_KHR, ctx->display, NULL);
        ERRCHECK_V(ctx->egl_display != EGL_NO_DISPLAY, *context->result, PX_FAILED_OGL_CONTEXT);
        ERRCHECK_V(eglInitialize(ctx->egl_display, NULL, NULL), *context->result, PX_FAILED_OGL_CONTEXT);
    }
    ERRCHECK_V(eglBindAPI(EGL_OPENGL_API), *context->result, PX_FAILED_OGL_CONTEXT);

    const EGLint config_attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_NONE
    };
    EGLConfig config;
    EGLint num_configs = 0;
    ERRCHECK_V(eglChooseConfig(ctx->egl_display, config_attribs, &config, 1, &num_configs), *context->result, PX_FAILED_VISUAL);
    ERRCHECK_V(num_configs > 0, *context->result, PX_FAILED_VISUAL);

    const EGLint context_attribs[] = {
        EGL_CONTEXT_MAJOR_VERSION, version_major,
        EGL_CONTEXT_MINOR_VERSION, version_minor,
        EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT,
        EGL_NONE
    };
    win->egl_context = eglCreateContext(ctx->egl_display, config, EGL_NO_CONTEXT, context_attribs);
    ERRCHECK_V(win->egl_context != EGL_NO_CONTEXT, *context->result, PX_FAILED_OGL_CONTEXT);

    win->egl_surface = eglCreateWindowSurface(ctx->egl_display, config, (EGLNativeWindowType)win->egl_window, NULL);
    ERRCHECK_V(win->egl_surface != EGL_NO_SURFACE, *context->result, PX_FAILED_OGL_CONTEXT);

    ERRCHECK_V(eglMakeCurrent(ctx->egl_display, win->egl_surface, win->egl_surface, win->egl_context), *context->result, PX_FAILED_OGL_CONTEXT);

    for (unsigned int i = 0; !libGL && i < sizeof(LIBGLNAMES) / sizeof(LIBGLNAMES[0]); i++) {
        libGL = dlopen(LIBGLNAMES[i], RTLD_NOW | RTLD_GLOBAL);
    }
    ERRCHECK_V(gladLoadGLLoader(PxiGLLoadproc), *context->result, PX_FAILED_FUNCTION_FETCH);
}

void PxSwapBuffers(PxWindow *window) {
    window_t *win = (window_t*)window->inner;
    context_t *ctx = (context_t*)window->ctx->inner;
    ERRCHECK_V(win->egl_surface != EGL_NO_SURFACE, *window->ctx->result, PX_FAILED_OGL_CONTEXT);
    ERRCHECK_V(eglSwapBuffers(ctx->egl_display, win->egl_surface), *window->ctx->result, PX_FAILED_OGL_CONTEXT);
}

void PxDestroyWindow(PxWindow *window) {
    window_t *win = (window_t*)window->inner;
    context_t *ctx = (context_t*)window->ctx->inner;

    if (ctx->egl_display != EGL_NO_DISPLAY) {
        eglMakeCurrent(ctx->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (win->egl_surface != EGL_NO_SURFACE) eglDestroySurface(ctx->egl_display, win->egl_surface);
        if (win->egl_context != EGL_NO_CONTEXT) eglDestroyContext(ctx->egl_display, win->egl_context);
    }
    if (win->egl_window) wl_egl_window_destroy(win->egl_window);

    if (win->xdg_toplevel) xdg_toplevel_destroy(win->xdg_toplevel);
    if (win->xdg_surf) xdg_surface_destroy(win->xdg_surf);
    if (win->surf) wl_surface_destroy(win->surf);
    if (ctx->kb_focus == window) {
        ctx->kb_focus = NULL;
        stop_key_repeat(ctx);
    }
    wl_display_flush(ctx->display);

    if (window->ecache.data) PxFree(window->ecache.data);
    if (window->info.title) PxFree(window->info.title);
    PxFree(window->inner);
    PxFree(window);
}

void PxDestroyContext(PxContext *context) {
    context_t *ctx = (context_t*)context->inner;
    if (ctx->egl_display != EGL_NO_DISPLAY) eglTerminate(ctx->egl_display);
    if (ctx->keyboard) wl_keyboard_release(ctx->keyboard);
    if (ctx->pointer) wl_pointer_release(ctx->pointer);
    if (ctx->seat) wl_seat_destroy(ctx->seat);
    if (ctx->xkb_state) xkb_state_unref(ctx->xkb_state);
    if (ctx->xkb_keymap) xkb_keymap_unref(ctx->xkb_keymap);
    if (ctx->xkb_context) xkb_context_unref(ctx->xkb_context);
    if (ctx->xdg_wm_base) xdg_wm_base_destroy(ctx->xdg_wm_base);
    if (ctx->compositor) wl_compositor_destroy(ctx->compositor);
    if (ctx->registry) wl_registry_destroy(ctx->registry);
    wl_display_disconnect(ctx->display);
    PxFree(context->inner);
    PxFree(context);
}

int PxiTransmogKeycode(int keycode) {
    switch (keycode) {
        case XKB_KEY_space: return PX_KEY_SPACE;
        case XKB_KEY_0: return PX_KEY_0;
        case XKB_KEY_1: return PX_KEY_1;
        case XKB_KEY_2: return PX_KEY_2;
        case XKB_KEY_3: return PX_KEY_3;
        case XKB_KEY_4: return PX_KEY_4;
        case XKB_KEY_5: return PX_KEY_5;
        case XKB_KEY_6: return PX_KEY_6;
        case XKB_KEY_7: return PX_KEY_7;
        case XKB_KEY_8: return PX_KEY_8;
        case XKB_KEY_9: return PX_KEY_9;

        case XKB_KEY_KP_0: return PX_KEY_NP_0;
        case XKB_KEY_KP_1: return PX_KEY_NP_1;
        case XKB_KEY_KP_2: return PX_KEY_NP_2;
        case XKB_KEY_KP_3: return PX_KEY_NP_3;
        case XKB_KEY_KP_4: return PX_KEY_NP_4;
        case XKB_KEY_KP_5: return PX_KEY_NP_5;
        case XKB_KEY_KP_6: return PX_KEY_NP_6;
        case XKB_KEY_KP_7: return PX_KEY_NP_7;
        case XKB_KEY_KP_8: return PX_KEY_NP_8;
        case XKB_KEY_KP_9: return PX_KEY_NP_9;


        case XKB_KEY_a: case XKB_KEY_A: return PX_KEY_A;
        case XKB_KEY_b: case XKB_KEY_B: return PX_KEY_B;
        case XKB_KEY_c: case XKB_KEY_C: return PX_KEY_C;
        case XKB_KEY_d: case XKB_KEY_D: return PX_KEY_D;
        case XKB_KEY_e: case XKB_KEY_E: return PX_KEY_E;
        case XKB_KEY_f: case XKB_KEY_F: return PX_KEY_F;
        case XKB_KEY_g: case XKB_KEY_G: return PX_KEY_G;
        case XKB_KEY_h: case XKB_KEY_H: return PX_KEY_H;
        case XKB_KEY_i: case XKB_KEY_I: return PX_KEY_I;
        case XKB_KEY_j: case XKB_KEY_J: return PX_KEY_J;
        case XKB_KEY_k: case XKB_KEY_K: return PX_KEY_K;
        case XKB_KEY_l: case XKB_KEY_L: return PX_KEY_L;
        case XKB_KEY_m: case XKB_KEY_M: return PX_KEY_M;
        case XKB_KEY_n: case XKB_KEY_N: return PX_KEY_N;
        case XKB_KEY_o: case XKB_KEY_O: return PX_KEY_O;
        case XKB_KEY_p: case XKB_KEY_P: return PX_KEY_P;
        case XKB_KEY_q: case XKB_KEY_Q: return PX_KEY_Q;
        case XKB_KEY_r: case XKB_KEY_R: return PX_KEY_R;
        case XKB_KEY_s: case XKB_KEY_S: return PX_KEY_S;
        case XKB_KEY_t: case XKB_KEY_T: return PX_KEY_T;
        case XKB_KEY_u: case XKB_KEY_U: return PX_KEY_U;
        case XKB_KEY_v: case XKB_KEY_V: return PX_KEY_V;
        case XKB_KEY_w: case XKB_KEY_W: return PX_KEY_W;
        case XKB_KEY_x: case XKB_KEY_X: return PX_KEY_X;
        case XKB_KEY_y: case XKB_KEY_Y: return PX_KEY_Y;
        case XKB_KEY_z: case XKB_KEY_Z: return PX_KEY_Z;

        case XKB_KEY_Home: return PX_KEY_HOME;
        case XKB_KEY_End: return PX_KEY_END;
        case XKB_KEY_Left: return PX_KEY_LEFT;
        case XKB_KEY_Right: return PX_KEY_RIGHT;
        case XKB_KEY_Up: return PX_KEY_UP;
        case XKB_KEY_Down: return PX_KEY_DOWN;
        case XKB_KEY_Page_Up: return PX_KEY_PAGE_UP;
        case XKB_KEY_Page_Down: return PX_KEY_PAGE_DOWN;

        default: return PX_KEY_UNKNOWN;
    }
}
