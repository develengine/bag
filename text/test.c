#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "bag_engine.h"
#include "bag_keys.h"

#include "bag_text.h"

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

    bagE_setWindowTitle("Font rendering test");

    int winWidth, winHeight;
    bagE_getWindowSize(&winWidth, &winHeight);


    const char *testString = "ÔMĚGÁ Ĺ Ý Ľ";
    const char *testString2 = "This is my kingdom come, this is my kingdom come.";
    int testLength = bagT_UTF8Length(testString);
    const char *testString3 = "Omega L Y L";
    int testLength3 = bagT_UTF8Length(testString3);


    if (bagT_init(winWidth, winHeight)){
        fprintf(stderr, "Failed to load bag font!\n");
        return -1;
    }


    bagT_Font *font = bagT_initFont("JetBrainsMono/regular.ttf", 0);
    if (!font) {
        fprintf(stderr, "Failed to load font!\n");
        return -1;
    }

    bagT_Instance *instance = bagT_instantiate(font, 25);
    if (!instance) {
        fprintf(stderr, "Failed to instantiate font!\n");
        return -1;
    }

    bagT_Instance *instance2 = bagT_instantiate(font, 20);
    if (!instance2) {
        fprintf(stderr, "Failed to instantiate font!\n");
        return -1;
    }


    bagT_Font *font2 = bagT_initFont("riglia/font.ttf", 0);
    if (!font2) {
        fprintf(stderr, "Failed to load font!\n");
        return -1;
    }

    bagT_Instance *instance3 = bagT_instantiate(font2, 25);
    if (!instance3) {
        fprintf(stderr, "Failed to instantiate font!\n");
        return -1;
    }


    bagT_Char *chars = malloc(testLength3 * sizeof(bagT_Char));
    int *indices = malloc(testLength3 * sizeof(int));
    int offset = 0;
    for (int i = 0; i < testLength; i++) {
        int move;
        indices[i] = bagT_UTF8ToGlyphIndex(instance3, testString3 + offset, &move);
        offset += move;
    }
    bagT_simpleCompositor(instance3, NULL, chars, indices, testLength3);


    bagT_Memory *memory = bagT_allocateMemory(instance3, chars, testLength3, bagT_StaticMemory);
    if (!memory) {
        fprintf(stderr, "Failed to allocate memory!\n");
        return -1;
    }

    bagT_Memory *memory2 = bagT_allocateMemory(instance3, NULL, testLength3 * 2, bagT_StaticMemory);
    if (!memory2) {
        fprintf(stderr, "Failed to allocate memory!\n");
        return -1;
    }

    bagT_openMemory(memory2);

    bagT_fillMemory(chars, 0, testLength3);

    int testWidth = chars[testLength3 - 1].x + bagT_getAdvance(instance3, chars[testLength3 - 1].glyphIndex);
    for (int i = 0; i < testLength3; i++) {
        chars[i].x += testWidth;
    }
    bagT_fillMemory(chars, testLength3, testLength3);

    bagT_closeMemory();


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

    time_t t = 0;
    int frames = 0;

    int counter = 0;

    bagT_Color red = { 1.0f, 0.0f, 0.0f, 1.0f };
    bagT_Color green = { 0.0f, 1.0f, 0.0f, 1.0f };
    bagT_Color blue = { 0.2f, 0.4f, 1.0f, 1.0f };

    bagT_Transform transform;
    transform.x = 100;

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


        bagT_useProgram(bagT_SimpleProgram);

        bagT_bindInstance(instance);

        transform.y = 100;
        transform.w = 3.0f;
        transform.h = 3.0f;
        bagT_renderUTF8String(testString, transform, NULL, &red);

        transform.y = 200;
        transform.w = 1.0f;
        transform.h = 1.0f;
        bagT_renderUTF8String(testString2, transform, NULL, &green);

        bagT_bindInstance(instance2);

        transform.y = 500;
        bagT_renderUTF8String(testString2, transform, NULL, &blue);


        bagT_useProgram(bagT_MemoryProgram);

        bagT_bindInstance(instance3);

        bagT_bindMemory(memory);

        transform.y = 300;
        bagT_renderMemory(0, (counter / 8) % (testLength + 1), transform);

        transform.y = 400;
        transform.w = 2.0f;
        transform.h = 2.0f;
        bagT_renderMemory((counter / 8) % testLength, 1, transform);

        bagT_bindMemory(memory2);

        transform.y = 600;
        transform.w = 1.0f;
        transform.h = 1.0f;
        bagT_renderMemory(0, testLength3 * 2, transform);

        bagT_unbindMemory();

        bagT_useProgram(bagT_NoProgram);

        
        bagE_swapBuffers();

        ++counter;
    }

    bagT_useProgram(bagT_NoProgram);

    bagT_freeMemory(memory);
    bagT_freeMemory(memory2);
    bagT_destroyInstance(instance);
    bagT_destroyInstance(instance2);
    bagT_destroyInstance(instance3);
    bagT_destroyFont(font);
    bagT_destroyFont(font2);
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
