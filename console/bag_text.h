#ifndef BAG_TEXT_H
#define BAG_TEXT_H

#define BAGT_MAX_RENDER_STRING_LENGTH 1024

typedef enum
{
    bagT_BasicCompositor,
    bagT_AdvancedCompositor
} bagT_Compositor;


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

typedef struct
{
    int x, y;
    int color;
    int glyphIndex;
} bagT_Char;


int bagT_init(int screenWidth, int screenHeight);
void bagT_updateResolution(int screenWidth, int screenHeight);
void bagT_destroy();

int bagT_initFont(bagT_Font *font, const char *path, float fontSize, int index);
void bagT_destroyFont(bagT_Font *font);

int bagT_allocateMemory(bagT_Font *font, bagT_Memory *memory, int length, bagT_MemoryType type);
int bagT_fillMemory(bagT_Font *font, bagT_Memory *memory, int offset, bagT_Char *chars, int length);
int bagT_destroyMemory(bagT_Memory *memory);

int bagT_codepointToGlyphIndex(bagT_Font *font, int codepoint);
int bagT_UTF8ToGlyphIndex(bagT_Font *font, const unsigned char *ch, int *offset);
float bagT_getAdvance(bagT_Font *font, int glyphIndex);
float bagT_getKerning(bagT_Font *font, int glyphIndex);
float bagT_getVerticalOffset(bagT_Font *font);

int bagT_UTF8Length(const unsigned char *string);

int bagT_renderUTF8String(
        bagT_Font *font,
        const char *string,
        int x, int y,
        bagT_Compositor compositor,
        int maxLength
);

int bagT_renderCodepoints(
        bagT_Font *font,
        int *codepoints,
        int count,
        int x, int y,
        bagT_Compositor compositor,
        int maxLength
);

int bagT_renderChars(bagT_Font *font, bagT_Char *chars, int count, int x, int y);

int bagT_renderMemory(bagT_Font *font, bagT_Memory *memory, int index, int count, int x, int y);

#endif
