#include <stdio.h>
#include <stdlib.h>
#include <pine.h>


#define ERRCHECK(var_to_check, res) if (!var_to_check) {printf("[ERROR]: %s\n", PxErrorToString(res)); exit(res);}

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

    while (!PxGetWindowParamI(context, win, PX_PARAM_SHOULD_CLOSE)) {
        PxPollEvents(win);
    }

    PxDestroyContext(context);
    PxDestroyWindow(win);
    return 0;
}