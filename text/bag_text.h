#ifndef BAG_TEXT_H
#define BAG_TEXT_H

/* config */

#define BAGT_MAX_RENDER_STRING_LENGTH 512  // change in simple shader as well
#define BAGT_GL_PATH <glad/gl.h>

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

struct bagT_Memory;
typedef struct bagT_Memory bagT_Memory;

struct bagT_Instance;
typedef struct bagT_Instance bagT_Instance;

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

typedef void (*bagT_Compositor) (void*, bagT_Char *, int*, int);


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

int bagT_allocateMemory(bagT_Memory **memory, int length, bagT_MemoryType type);
int bagT_fillMemory(bagT_Memory *memory, bagT_Char *chars, int offset, int length);
void bagT_bindMemory(bagT_Memory *memory);
int bagT_freeMemory(bagT_Memory *memory);

int bagT_codepointToGlyphIndex(bagT_Instance *instance, int codepoint);
int bagT_UTF8ToGlyphIndex(bagT_Instance *instance, const unsigned char *ch, int *offset);
void bagT_getOffset(bagT_Instance *instance, int glyphIndex, int *x, int *y);
float bagT_getAdvance(bagT_Instance *instance, int glyphIndex);
float bagT_getKerning(bagT_Instance *instance, int glyphIndex1, int glyphIndex2);
bagT_VMetrics bagT_getVMetrics(bagT_Instance *instance);

int bagT_UTF8Length(const unsigned char *string);

void bagT_renderUTF8String(
        const char *string,
        int x, int y,
        bagT_Compositor compositor,
        void *compositorData
);

void bagT_renderCodepoints(
        int *codepoints,
        int count,
        int x, int y,
        bagT_Compositor compositor,
        void *compositorData
);

void bagT_renderChars(bagT_Char *chars, int count, int x, int y);

int bagT_renderMemory(int index, int count, int x, int y);

#endif
