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


typedef struct
{

} bagT_Font;


typedef struct
{

} bagT_Memory;


typedef struct
{
    int x, y;
    int glyphIndex;
    unsigned int color;
} bagT_Char;


int bagT_init(bagT_Font *font, const char *dir, int size, int index);
void bagT_updateResolition(bagT_Font *font, int width, int height);
void bagT_destroy(bagT_Font *font);

int bagT_allocateMemory(bagT_Memory *memory, int length, bagT_MemoryType type);
int bagT_fillMemory(bagT_Memory *memory, int offset, bagT_Char *chars, int length);
int bagT_destroyMemory(bagT_Memory *memory);

int bagT_codepointToGlyphIndex(bagT_Font *font, int codepoint);
int bagT_UTF8ToGlyphIndex(bagT_Font *font, const char *ch, int *offset);
int bagT_UTF8Length(const char *string);


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
