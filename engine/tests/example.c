#include <stdio.h>
#include <stdlib.h>

#include "bag_engine.h"
#include "bag_keys.h"

static int running = 1;
static int altDown = 0;
static int shiftDown = 0;


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


int bagE_main(int argc, char *argv[])
{
    bagE_setWindowTitle("Example program");

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(openglCallback, 0); 

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

        case bagE_EventWindowResize: {
            bagE_WindowResize *wr = &(event->data.windowResize);
            glViewport(0, 0, wr->width, wr->height);
        } break;

        default: break;
    }

    return 0;
}
