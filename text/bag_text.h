#ifndef BAG_TEXT_H
#define BAG_TEXT_H

/* config */

#define BAGT_MAX_RENDER_STRING_LENGTH 512  // change in simple shader as well

#include <glad/gl.h>
/* specification */

typedef enum
{
    bagT_NoProgram,
    bagT_SimpleProgram,
    bagT_MemoryProgram
} bagT_Program;

typedef enum
{
    bagT_StaticMemory,
    bagT_DynamicMemory,
    bagT_StreamMemory
} bagT_MemoryType;


struct bagT_Font;
typedef struct bagT_Font bagT_Font;

struct bagT_Instance;
typedef struct bagT_Instance bagT_Instance;

struct bagT_Memory;
typedef struct bagT_Memory bagT_Memory;

typedef struct
{
    int x : 32;
    int y : 32;
    int color : 32;
    int glyphIndex : 32;
} bagT_Char;

typedef struct
{
    float ascent;
    float descent;
    float lineGap;
} bagT_VMetrics;

typedef struct
{
    float r, g, b, a;
} bagT_Color;

typedef struct
{
    int x, y;
    float w, h;
    float r;
} bagT_Transform;


typedef void (*bagT_Compositor) (bagT_Instance*, void*, bagT_Char*, int*, int);


int bagT_init(int screenWidth, int screenHeight);
void bagT_updateResolution(int screenWidth, int screenHeight);
void bagT_useProgram(bagT_Program program);
void bagT_destroy();

bagT_Font *bagT_initFont(const char *path, int index);
bagT_Instance *bagT_instantiate(bagT_Font *font, float fontSize);
void bagT_bindInstance(bagT_Instance *instance);
void bagT_unbindInstance();
void bagT_destroyFont(bagT_Font *font);
void bagT_destroyInstance(bagT_Instance *instance);

void bagT_createInstanceFile(bagT_Font *font, float fontSize, const char *fileName);
bagT_Instance *bagT_loadInstanceFile(bagT_Font *font, const char *fileName);

bagT_Memory *bagT_allocateMemory(
        bagT_Instance *instance,
        bagT_Char *data,
        int count,
        bagT_MemoryType type
);

void bagT_openMemory(bagT_Memory *memory);
void bagT_fillMemory(bagT_Char *chars, int offset, int length);
void bagT_closeMemory();
void bagT_bindMemory(bagT_Memory *memory);
void bagT_unbindMemory();
void bagT_freeMemory(bagT_Memory *memory);

int bagT_codepointToGlyphIndex(bagT_Instance *instance, int codepoint);
int bagT_UTF8ToGlyphIndex(bagT_Instance *instance, const char *ch, int *offset);
void bagT_getOffset(bagT_Instance *instance, int glyphIndex, int *x, int *y);
float bagT_getAdvance(bagT_Instance *instance, int glyphIndex);
float bagT_getKerning(bagT_Instance *instance, int glyphIndex1, int glyphIndex2);
bagT_VMetrics bagT_getVMetrics(bagT_Instance *instance);

int bagT_UTF8Length(const char *string);

void bagT_renderUTF8String(
        const char *string,
        bagT_Transform transform,
        bagT_Compositor compositor,
        void *compositorData
);

void bagT_renderCodepoints(
        int *codepoints,
        int count,
        bagT_Transform transform,
        bagT_Compositor compositor,
        void *compositorData
);

void bagT_renderChars(bagT_Char *chars, int count, bagT_Transform transform);

void bagT_renderMemory(int index, int count, bagT_Transform transform);

void bagT_simpleCompositor(bagT_Instance *instance, void *data, bagT_Char *chars, int *glyphIndices, int length);

#endif
