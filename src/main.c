#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <pine.h>

#include <GL/glu.h>


#define ERRCHECK(var_to_check, res) if (!var_to_check) {printf("[ERROR]: %s\n", PxErrorToString(res)); exit(res);}
#define ERRCHECK_(res) if (res) {printf("[ERROR]: %s\n", PxErrorToString(res)); exit(res);}

const PxWindowInfo params = {
    1080, 720,
    100, 100,
    .title = "Hello, World!"
};

int main() {
    PxResult res = 0;
    PxContext *context = PxCreateContext(&res);
    ERRCHECK(context, res)

    PxWindow *win = PxCreateWindow(context, params, NULL);
    ERRCHECK(win, res)

    PxLoadOpenGL(context, win, 4, 3);

    PxEvent event = {0};
    int running = 1;
    while (running) {
        while (PxPollEvents(win, &event)) {
            switch (event.type) {
                case PX_EVENT_WINDOW_MOVE:
                    int *pos = (int*)PxGetWindowParam(context, win, PX_PARAM_POSITION);
                    printf("Window moved! %i, %i\n", pos[0], pos[1]);
                    break;

                case PX_EVENT_RESIZE:
                    int *size = (int*)PxGetWindowParam(context, win, PX_PARAM_SIZE);
                    glViewport(0, 0, size[0], size[1]);
                    printf("Window resized! %i, %i\n", size[0], size[1]);
                    break;

                case PX_EVENT_KEYUP:
                    if (event.keycode == PX_KEY_UNKNOWN) {printf("ERROR: Unknown keycode (%i)\n", (int)event.keycode); break;}
                    printf("keyup: %i: %c\n", event.keycode, (char)event.keycode);
                    break;

                case PX_EVENT_CLOSE:
                    running = 0;
                    break;

                default: break;
            }
        }

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        PxSwapBuffers(win);
    }

    PxDestroyWindow(win);
    PxDestroyContext(context);
    return 0;
}
