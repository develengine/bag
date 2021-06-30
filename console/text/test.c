#include <stdio.h>
#include <stdlib.h>

#include "bag_engine.h"
#include "bag_keys.h"

#include "bag_text.h"

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
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(openglCallback, 0); 

    bagE_setWindowTitle("Font rendering test");

    int winWidth, winHeight;
    bagE_getWindowSize(&winWidth, &winHeight);

    if (bagT_init(winWidth, winHeight)){
        fprintf(stderr, "Failed to load bag font!\n");
        return -1;
    }

    bagT_Font *font;
    if (bagT_initFont(&font, "JetBrainsMono/regular.ttf", 20, 0)) {
        fprintf(stderr, "Failed to load font!\n");
        return -1;
    }

    int maxUniformBlockSize;
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUniformBlockSize);
    printf("mubs: %d\n", maxUniformBlockSize);

    bagE_setSwapInterval(1);

    while (running) {
        bagE_pollEvents();

        if (!running)
            break;

#if 1
        glClearColor(
                0x0d / 255.f,
                0x11 / 255.f,
                0x17 / 255.f,
                1.0f
        );
#else
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
#endif
        glClear(GL_COLOR_BUFFER_BIT);

        bagE_swapBuffers();
    }

    bagT_destroyFont(font);
    bagT_destroy();

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
            bagT_updateResolution(wr->width, wr->height);
        } break;

        default: break;
    }

    return 0;
}
