#ifndef BAG_ENGINE_H
#define BAG_ENGINE_H

#include <glad/gl.h>

#define BAGE_TEXT_SIZE 12

/* TODO
 * [ ] static functionality enabling
 * [ ] dynamic functionality enabling
 * [ ] opengl version selection
 * [ ] separate cursor hiding and restricting into two functions
 * [ ] create a vsync get function
 * [ ] add a blocking event handling routine
 */

typedef enum
{
    bagE_ButtonLeft = 1,
    bagE_ButtonMiddle,
    bagE_ButtonRight,

    bagE_ButtonCount
} bagE_Button;

typedef enum
{
    bagE_EventNone,

    bagE_EventWindowClose,
    bagE_EventWindowResize,

    bagE_EventMouseMotion,
    bagE_EventMouseButtonUp,
    bagE_EventMouseButtonDown,
    bagE_EventMouseWheel,
    bagE_EventMousePosition,

    bagE_EventKeyUp,
    bagE_EventKeyDown,
    bagE_EventTextUTF8,
    bagE_EventTextUTF32,

    bagE_EventTypeCount
} bagE_EventType;


typedef struct
{
    int width;
    int height;
} bagE_WindowResize;

typedef struct
{
    int x, y;
} bagE_Mouse;

typedef struct
{
    float x, y;
} bagE_MouseMotion;

typedef struct
{
    int x, y;
    bagE_Button button;
} bagE_MouseButton;

typedef struct
{
    int x, y;
    int scrollUp;
} bagE_MouseWheel;

typedef struct
{
    unsigned int key;
    int repeat;
} bagE_Key;

typedef struct
{
    unsigned char text[BAGE_TEXT_SIZE];
} bagE_TextUTF8;

typedef  struct
{
    unsigned int codePoint;
} bagE_TextUTF32;


typedef struct
{
    bagE_EventType type;

    union
    {
        bagE_WindowResize windowResize;
        bagE_Mouse        mouse;
        bagE_MouseMotion  mouseMotion;
        bagE_MouseButton  mouseButton;
        bagE_MouseWheel   mouseWheel;
        bagE_Key          key;
        bagE_TextUTF8     textUTF8;
        bagE_TextUTF32    textUTF32;
    } data;
} bagE_Event;


void bagE_pollEvents();
void bagE_swapBuffers();

int bagE_getCursorPosition(int *x, int *y);
void bagE_getWindowSize(int *width, int *height);

int bagE_isAdaptiveVsyncAvailable(void);

int bagE_setHiddenCursor(int value);
void bagE_setFullscreen(int value);
void bagE_setWindowTitle(char *value);
void bagE_setSwapInterval(int value);
void bagE_setCursorPosition(int x, int y);

/* User defined */
int bagE_eventHandler(bagE_Event *event);
int bagE_main(int argc, char *argv[]);

#endif
