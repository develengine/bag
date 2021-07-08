#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

    fprintf(stderr, "[%s] type: %d, severity: %d\n%s\n",
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

    bagT_Font *font = bagT_initFont("JetBrainsMono/regular.ttf", 0);
    if (!font) {
        fprintf(stderr, "Failed to load font!\n");
        return -1;
    }

    bagT_Instance *instance = bagT_instantiate(font, 20);
    if (!instance) {
        fprintf(stderr, "Failed to instantiate font!\n");
        return -1;
    }

    const char *testString = "ÔMĚGÁ Ĺ Ý Ľ";

    printf("strlen: %ld\n", strlen(testString));
    printf("UTF8len: %d\n", bagT_UTF8Length((const unsigned char*)testString));

    int maxUniformBlockSize;
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUniformBlockSize);
    printf("mubs: %d\n", maxUniformBlockSize);

    bagE_setSwapInterval(1);

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

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    bagT_setColor(0.1f, 1.0f, 0.1f, 1.0f);

    bagT_bindInstance(instance);
    bagT_useProgram(bagT_SimpleProgram);

    time_t t = 0;
    int frames = 0;

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

        bagT_renderUTF8String(testString, 100, 100, NULL, NULL);

        bagE_swapBuffers();
    }

    bagT_useProgram(bagT_NoProgram);

    bagT_destroyInstance(instance);
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
