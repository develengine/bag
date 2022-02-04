#ifndef BAG_CONSOLE_H
#define BAG_CONSOLE_H

#include "bag_text.h"

#define BAGC_FONT_FILE_NAME "jbm.ttf"
#define BAGC_FONT_INSTANCE_FILE_NAME "bagc-font-instance"
#define BAGC_FONT_SIZE 25

typedef enum
{
    bagC_Enter,
    bagC_Reset,

    bagC_Delete,
    bagC_BackSpace,

    bagC_Home,
    bagC_End,

    bagC_Forth,
    bagC_Back,

    bagC_Next,
    bagC_Previous,

    bagC_CommandCount
} bagC_Command;


typedef void (*bagC_InputCallback) (const char *);


typedef struct
{
    int bufferLineCount;
    int maxLineWidth;
    int inputBufferLength;

    bagC_InputCallback callback;
    int *inputBuffer;
    int **lineBuffer;
    bagT_Memory *memory;
    char *name;

    int start;
    int count;
} bagC_Buffer;

typedef struct
{
    char *name;
    int bufferLineCount;
    int maxLineWidth;
    int inputBufferLength;
    bagC_InputCallback callback;
} bagC_BufferInfo;


int bagC_init(int windowWidth, int windowHeight);
void bagC_hide();
void bagC_show();
void bagC_render(float deltaTime);
void bagC_destroy();

void bagC_updateResolution(int windowWidth, int windowHeight);

int bagC_createBuffer(bagC_Buffer *buffer, const bagC_BufferInfo *info);
void bagC_bindBuffer(bagC_Buffer *buffer);
void bagC_freeBuffer(bagC_Buffer *buffer);

void bagC_writeLine(bagC_Buffer *buffer, const char *string);
void bagC_clear(bagC_Buffer *buffer);
void bagC_scroll(int amount);

void bagC_input(const char *string);
void bagC_command(bagC_Command command);

#endif
