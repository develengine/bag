#ifndef BAG_CONSOLE_H
#define BAG_CONSOLE_H

#include "bag_text.h"

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


typedef void (*bagC_InputCallback) (char *);


typedef struct
{
    int bufferLineCount;
    int maxLineWidth;
    bagC_InputCallback inputCallback;
    char *inputBuffer;
    char **dataBuffer;
    bagT_Memory *memory;
} bagC_Buffer;


int bagC_init(int windowWidth, int windowHeight);
void bagC_hide();
void bagC_show();
void bagC_render(float deltaTime);
void bagC_destroy();

void bagC_updateResolution(int windowWidth, int windowHeight);

bagC_Buffer *bagC_createBuffer(const char *name, int bufferLineCount, int maxLineWidth);
void bagC_bindBuffer(const bagC_Buffer *buffer);
void bagC_freeBuffer(bagC_Buffer *buffer);

void bagC_writeLine(bagC_Buffer *buffer, const char *string);
void bagC_clear(bagC_Buffer *buffer);
void bagC_scroll(int amount);

void bagC_input(const char *string);
void bagC_command(bagC_Command command);

#endif
