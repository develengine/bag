#include <stdio.h>
#include <stdlib.h>

#include "bag_engine.h"
#include "bag_keys.h"

#define MODERN_GL 0


static int running = 1;


#if MODERN_GL
void GLAPIENTRY openglCallback(
        GLenum source,
        GLenum type,
        GLuint id,
        GLenum severity,
        GLsizei length,
        const GLchar* message,
        const void* userParam
) {
    int error = type == GL_DEBUG_TYPE_ERROR;

    printf("[%s] type: %d, severity: %d\n%s\n",
            error ? "\033[1;31mERROR\033[0m" : "\033[1mINFO\033[0m",
            type, severity, message
    );
}
#endif


int bagE_main(int argc, char *argv[])
{
#if MODERN_GL
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(openglCallback, 0); 
#endif

    FILE *parameters = fopen("parameters.txt", "w");
    for (int i = 0; i < argc; i++)
        fprintf(parameters, "%d: %s\n", i, argv[i]);
    fclose(parameters);

    const char *vendorString = (const char*)glGetString(GL_VENDOR);
    const char *rendererString = (const char*)glGetString(GL_RENDERER);
    const char *versionString = (const char*)glGetString(GL_VERSION);
    const char *shadingLanguageVersionString = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    printf("Vendor: %s\nRenderer: %s\nVersion: %s\nShading Language version: %s\n",
        vendorString, rendererString, versionString, shadingLanguageVersionString);

    while (running) {
        bagE_pollEvents();

        if (!running)
            break;

        glClearColor(
                (float)(rand() % 256) / 255.f,
                (float)(rand() % 256) / 255.f,
                (float)(rand() % 256) / 255.f,
                1.0f
        );
        glClear(GL_COLOR_BUFFER_BIT);

        bagE_swapBuffers();
    }

    return 0;
}

int bagE_eventHandler(bagE_Event *event)
{
    switch (event->type)
    {
        case bagE_EventWindowClose:
            running = 0;
            return 1;

        case bagE_EventWindowResize:
            bagE_WindowResize *wr = &(event->data.windowResize);
            printf("resize w: %d, h: %d\n",
                    event->data.windowResize.width,
                    event->data.windowResize.height);
            glViewport(0, 0, wr->width, wr->height);
            break;
    }

    return 0;
}
