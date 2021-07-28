#include <stdio.h>
#include <time.h>

#include "bag_engine.h"
#include "bag_keys.h"
#include "bag_text.h"
#include "bag_console.h"

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

    fprintf(stderr, "[%s] type: %d, severity: %d\n%s\n",
            error ? "\033[1;31mERROR\033[0m" : "\033[1mINFO\033[0m",
            type, severity, message
    );
}


int bagE_main(int argc, char *argv[])
{
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(openglCallback, 0); 

    bagE_setWindowTitle("Console test");

    int winWidth, winHeight;
    bagE_getWindowSize(&winWidth, &winHeight);

    if (bagT_init(winWidth, winHeight)){
        fprintf(stderr, "Failed to load bag font!\n");
        return -1;
    }

    if (bagC_init(winWidth, winHeight)) {
        fprintf(stderr, "Failed to initialize console!\n");
        return -1;
    }

    bagE_setSwapInterval(1);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    time_t t = 0;
    int frames = 0;

    int counter = 0;

    while (running) {
        bagE_pollEvents();

        if (!running)
            break;

        time_t nt;
        time(&nt);
        if (nt != t) {
            printf("frames: %d\n", frames);
            frames = 0;
            t = nt;
        }
        ++frames;

        glClear(GL_COLOR_BUFFER_BIT);
        
        bagC_render(1.0f);

        bagE_swapBuffers();

        ++counter;
    }

    bagC_destroy();
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
            bagC_updateResolution(wr->width, wr->height);
        } break;

        default: break;
    }

    return 0;
}

