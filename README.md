# Pine - Homebrew cross-platform OpenGL loader

Pine is a simple cross-platform OpenGL loader focused on doing the bare minimum
while doing a bit more than your average loader like GLFW.

One of these goals is to abstract away the OpenGL function loading through embedding
GLAD instead of making you add it yourself. For most usecases this is fine, but if you
want more granular control that is also possible.

Pine supports both Wayland and X11 aswell as Win32. Mac compatability is planned but
wont be implemented before i get my hands on one.

# Usage

## Creating a window
The following example demonstrates how to initialize Pine, open a window and make
sure it closes properly aswell.

```C
#include <pine/pine.h>
#include <stddef.h>

// Define all parameters for window creation
const PxWindowInfo window_info = {
    .width = 1080, .height = 720,
    .x = 100, .y = 100,
    .title = "Hello, World!"
};

int main(void) {
    // The result is what Pine will use for error responses
    // to error check after a function call simply check
    // if result is non-zero. Ownership stays in with the
    // caller, which is why we pass a reference to PxCreateContext.
    PxResult result = 0;
    PxContext *context = PxCreateContext(&result);
    if (result) {
        fprintf(stderr, "ERROR: Failed to create a Pine context: %s", 
                PxErrorToString(result));
        return result;
    }

    // After a context has been created we can create a window
    // within the context. The context owns all windows created
    // with it and they will all inherit specific properties from
    // the context. It is also possible to have multiple contexts
    // per application and multiple windows per contexts.
    PxWindow *window = PxCreateWindow(context, window_info, NULL);
    if (result) {
        fprintf(stderr, "ERROR: Failed to create a window: %s", 
                PxErrorToString(result));
        return result;
    }

    int running = PX_TRUE;
    PxEvent event = {0};

    // Here we create the application mainloop, if you ever have used
    // PyGame this will look familiar.
    while (running) {
        while (PxPollEvents(window, &event)) {
            switch (event.type) {
                case PX_EVENT_CLOSE:
                    running = PX_FALSE;
                    break;

                default:
                    break;
            }
        }
        // Here is where you would be doing your draw calls
    }

    // Finally delete the window and then the context. In that order.
    // If the context outlives the window you will run into undefined
    // behaviour.
    PxDestroyWindow(window);
    PxDestroyContext(context);
}
```

### Loading OpenGL

After you have created a context and window everything is ready
for you to start using OpenGL. Pine provides the `PxLoadOpenGL`
helper function, or you may use any OpenGL function loader that
you may desire.

`PxLoadOpenGL` takes two parameters for the major and minor version 
you want to load.
This function will call the embedded build of GLAD to load all
OpenGL functions associated with the provided version of OpenGL.

# Events

![NOTE] Todo
