#include <pine/pine.h>
#include <stdio.h>



static const PxWindowInfo window_args = {
    .width = 1080, .height = 720,
    .x = 100, .y = 100,
    .title = "LearnOpenGL (Hello Triangle)"
};

int running = 1;

float vertices[] = {
    -0.5f, -0.5f, 0.0f,
     0.5f, -0.5f, 0.0f,
     0.0f,  0.5f, 0.0f
};  

const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "}\0";

const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
        "FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
    "}\0";

int main() {
    PxResult result = 0;
    PxContext *context = PxCreateContext(&result);
    if (!context) {
        fprintf(stderr, "ERROR: Failed to initialize PxContext: %s\n", PxErrorToString(result));
        goto ret;
    }
    
    PxWindow *window = PxCreateWindow(context, window_args, NULL);
    if (!window) {
        fprintf(stderr, "ERROR: Failed to initialize window: %s\n", PxErrorToString(result));
        goto destroy_context;
    }
    
    PxLoadOpenGL(context, window, 4, 3);
    if (result) {
        printf("ERROR: Failed to initialize OpenGL: %s\n", PxErrorToString(res));
    }

    gladLoadGL();

    glEnable(GL_DEPTH_TEST);
    
    unsigned int VBO, VAO;
    glGenBuffers(1, &VBO);
    glGenVertexArrays(1, &VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(VAO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    unsigned int vertexShader, fragmentShader, shaderProgram;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    shaderProgram = glCreateProgram();

    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);

    glCompileShader(vertexShader);
    glCompileShader(fragmentShader);

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    PxEvent event;
    while (running) {
        while (PxPollEvents(window, &event)) {
            switch (event.type) {
                case PX_EVENT_CLOSE:
                    running = 0;
            }
        }
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);


        PxSwapBuffers(window);
    }


    PxDestroyWindow(window);
destroy_context:
    PxDestroyContext(context);
ret:
    return result;
}