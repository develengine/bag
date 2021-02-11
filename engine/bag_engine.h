#ifndef BAG_ENGINE_H
#define BAG_ENGINE_H

#include <glad/gl.h>

typedef unsigned long bagE_ButtonMask;
#define BAGE_BUTTON_BIT_LEFT   0x01
#define BAGE_BUTTON_BIT_MIDDLE 0x02
#define BAGE_BUTTON_BIT_RIGHT  0x04

typedef unsigned long bagE_ModMask;
#define BAGE_MOD_BIT_SHIFT   0x01
#define BAGE_MOD_BIT_CONTROL 0x02
#define BAGE_MOD_BIT_ALT     0x04
#define BAGE_MOD_BIT_SUPER   0x08
#define BAGE_MOD_BIT_LOCK    0x10

#define BAGE_TEXT_SIZE 12


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
    bagE_ButtonMask buttons;
    bagE_ModMask modifiers;
    int x, y;
} bagE_Mouse;

typedef struct
{
    bagE_ButtonMask buttons;
    bagE_ModMask modifiers;
    float x, y;
} bagE_MouseMotion;

typedef struct
{
    bagE_ButtonMask buttons;
    bagE_ModMask modifiers;
    int x, y;
    bagE_Button button;
} bagE_MouseButton;

typedef struct
{
    bagE_ButtonMask buttons;
    bagE_ModMask modifiers;
    int x, y;
    int scrollUp;
} bagE_MouseWheel;

typedef struct
{
    bagE_ButtonMask buttons;
    bagE_ModMask modifiers;
    unsigned int key;
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
