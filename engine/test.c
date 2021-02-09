#include <stdio.h>
#include <stdlib.h>

#include "bag_engine.h"
#include "bag_keys.h"

static int running = 1;


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
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(openglCallback, 0); 

    const char *vendorString = (const char *)glGetString(GL_VENDOR);
    const char *rendererString = (const char *)glGetString(GL_RENDERER);
    const char *versionString = (const char *)glGetString(GL_VERSION);
    const char *shadingLanguageVersionString = (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
    printf("Vendor: %s\nRenderer: %s\nVersion: %s\nShading Language version: %s\n",
        vendorString, rendererString, versionString, shadingLanguageVersionString);

    printf("Adaptive: %d\n", bagE_isAdaptiveVsyncAvailable());
    bagE_setWindowTitle("a cool flippin new window");

    bagE_setSwapInterval(1);

    while (running) {
        bagE_pollEvents();

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        bagE_swapBuffers();
    }

    return 0;
}


void printBin(unsigned int num)
{
    char str[33];
    str[32] = 0;
    unsigned int mask = 0x80000000;

    for (int i = 0; i < 32; i++) {
        str[i] = '0' + ((mask & num) > 0);
        mask >>= 1;
    }

    printf("%s\n", str);
}


void bagE_eventHandler(bagE_Event *event)
{
    switch (event->type)
    {
        case bagE_EventWindowClose: {
            running = 0;
        } break;

        case bagE_EventWindowResize: {
            bagE_WindowResize *wr = &(event->data.windowResize);
            glViewport(0, 0, wr->width, wr->height);
        } break;

        case bagE_EventKeyDown: {
            if (event->data.key.key == KEY_F4) {
                printf("key down\n");
                printf("   mods: ");
                printBin(event->data.key.modifiers);
                printf("buttons: ");
                printBin(event->data.key.buttons);
            } else if (event->data.key.key == KEY_U) {
                int width, height;
                bagE_getWindowSize(&width, &height);
                printf("width: %d, height: %d\n", width, height);
                int onWindow = bagE_getCursorPosition(&width, &height);
                printf("cursor x: %d, y: %d, on window: %d\n", width, height, onWindow);
            } else if (event->data.key.key == KEY_P) {
                bagE_setCursorPosition(100, 100);
            } else if (event->data.key.key == KEY_F) {
                bagE_setFullscreen((event->data.key.modifiers & BAGE_MOD_BIT_SHIFT) == 0);
            } else if (event->data.key.key == KEY_C) {
                bagE_setHiddenCursor((event->data.key.modifiers & BAGE_MOD_BIT_SHIFT) == 0);
            }
        } break;

        case bagE_EventKeyUp: {
            if (event->data.key.key == KEY_SPACE) {
                printf("key up\n");
            }
        } break;
        
        default: break;
    }
}

