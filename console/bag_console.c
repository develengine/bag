#include <stdio.h>
#include <limits.h>
#include <stdint.h>

#include "bag_console.h"

/* TODO
 * [ ] proper error checking
 */

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


#define FAILED_FILE_IO() {\
    fprintf(stderr, "bag console: Failed file io.\n");\
    return -1;\
}


typedef struct
{
    int x, y;
    int w, h;
    int index;
} Rectangle;

#include "rect_tile.c"

typedef struct
{
    float x, y, w, h;
    float xOff, yOff;
} GlyphTransform;

void writeBitmap(
        uint8_t *src, uint8_t *dest,
        int sw, int sh, int dw, int dh,
        int x, int y
) {
    for (int i = 0; i < sh; i++) {
        for (int j = 0; j < sw; j++) {
            dest[(y + i) * dw + x + j] = src[i * sw + j];
        }
    }
}

const char *bagC_font = "JetBrainsMono-Regular.ttf";

int bagC_init()
{
    /* Load font file to memory */

    FILE *fontFile = fopen(bagC_font, "rb");
    if (fontFile == NULL) {
        fprintf(stderr, "bag console: Failed to load font file!\n");
        return -1;
    }

    if (fseek(fontFile, 0L, SEEK_END))
        FAILED_FILE_IO();

    int fontSize = ftell(fontFile);
    if (fontSize == -1L)
        FAILED_FILE_IO();

    rewind(fontFile);

    unsigned char *fontBuffer = malloc(fontSize);
    if (!fontBuffer) {
        fprintf(stderr, "bag console: Malloc fail!\n");
        return -1;
    }

    if (fread(fontBuffer, 1, fontSize, fontFile) != fontSize)
        FAILED_FILE_IO();

    if (fclose(fontFile))
        FAILED_FILE_IO();

    
    /* stb_truetype */

    stbtt_fontinfo fontInfo;  // """opaque""" type
    int ret = stbtt_InitFont(&fontInfo, fontBuffer, stbtt_GetFontOffsetForIndex(fontBuffer, 0));
    if (!ret) {
        fprintf(stderr, "bag console: Failed to load font from font file!\n");
        return -1;
    }

    printf("numGlyphs: %d\n", fontInfo.numGlyphs);
    unsigned char **bitmaps = malloc(fontInfo.numGlyphs * sizeof(unsigned char*));
    Rectangle *rects = malloc(fontInfo.numGlyphs * sizeof(Rectangle));
    if (!bitmaps || !rects) {
        fprintf(stderr, "bag console: Malloc fail!\n");
        return -1;
    }

    for (int i = 0; i < fontInfo.numGlyphs; i++) {
        int xOff, yOff, width, height;
        unsigned char *bitmap = stbtt_GetGlyphBitmap(
                &fontInfo,
                0,
                stbtt_ScaleForPixelHeight(&fontInfo, 12),
                i,
                &width, &height,
                &xOff, &yOff
        );

        if (!bitmap)
            fprintf(stderr, "glyph %d failed to load!, w: %d, h: %d\n", i, width, height);


        bitmaps[i] = bitmap;
        rects[i].x = 0;
        rects[i].y = 0;
        rects[i].w = width;
        rects[i].h = height;
        rects[i].index = i;
    }

    packRects(rects, fontInfo.numGlyphs);
    Span atlasSpan = getSpan(rects, fontInfo.numGlyphs);

    uint8_t *atlasBuffer = malloc(atlasSpan.w * atlasSpan.h);

    if (!atlasBuffer) {
        fprintf(stderr, "Failed malloc! Allocating glyph atlas buffer.\n");
        return -1;
    }

    memset(atlasBuffer, 0, atlasSpan.w * atlasSpan.h);

    for (int i = 0; i < fontInfo.numGlyphs; i++) {
        int bitmapIndex = rects[i].index;

        writeBitmap(
                bitmaps[bitmapIndex], atlasBuffer,
                rects[i].w, rects[i].h,
                atlasSpan.w, atlasSpan.h,
                rects[i].x, rects[i].y
        );

        stbtt_FreeBitmap(bitmaps[bitmapIndex], NULL);
    }

    printf("Atlas width: %d, height: %d\n", atlasSpan.w, atlasSpan.h);
    stbi_write_png("atlas.png", atlasSpan.w, atlasSpan.h, 1, atlasBuffer, atlasSpan.w);

    // free(atlasBuffer);
    free(rects);
    free(fontBuffer);


    


    return 0;
}

#undef FAILED_FILE_IO
