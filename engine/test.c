#include <stdio.h>
#include <stdlib.h>

#include "bag_engine.h"
#include "bag_keys.h"

static int running = 1;
static int printAbsolute = 0;
static int printRelative = 0;


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

        if (!running)
            break;

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

        case bagE_EventKeyDown: {
            printf("key down\n");

            if (event->data.key.key == KEY_DELETE) {
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
            } else if (event->data.key.key == KEY_M) {
                if (event->data.key.modifiers & BAGE_MOD_BIT_SHIFT) {
                    printRelative = !printRelative;
                } else {
                    printAbsolute = !printAbsolute;
                }
            }
        } break;

        case bagE_EventKeyUp:
            if (event->data.key.key == KEY_SPACE)
                printf("space up\n");
            break;
        
        case bagE_EventMouseWheel:
            printf("MODS SCROLL UP: %d\n", event->data.mouseWheel.scrollUp);
            break;

        case bagE_EventMouseButtonDown:
            printf("BUTTON DOWN: %d\n", event->data.mouseButton.button);
            break;

        case bagE_EventMouseButtonUp:
            printf("BUTTON UP: %d\n", event->data.mouseButton.button);
            break;

        case bagE_EventMousePosition: 
            if (printAbsolute) 
                printf("ABSOLUTE X: %3d, Y: %3d\n", event->data.mouse.x, event->data.mouse.y);
            break;

        case bagE_EventMouseMotion:
            if (printRelative)
                printf("RELATIVE X: %.2f, Y: %.2f\n", event->data.mouseMotion.x, event->data.mouseMotion.y);
            break;

        case bagE_EventTextUTF8:
            printf("text: %s\n", event->data.textUTF8.text);
            break;

        case bagE_EventTextUTF32:
            printf("codePoint: %x\n", event->data.textUTF32.codePoint);
            break;

        default: break;
    }

    return 0;
}

