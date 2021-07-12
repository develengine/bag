#include <stdio.h>
#include <limits.h>
#include <stdint.h>
#include <time.h>

#include "bag_engine.h"
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
    int flags;
    int index;
} Rectangle;

#include "rect_tile.c"

typedef struct
{
    int x, y, w, h;
    int xOff, yOff;
} Glyph;

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

static char *readFileToString(const char *path)
{
    FILE *file = fopen(path, "r");
    
    if (!file)
        return NULL;

    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    rewind(file);
    
    char *data = malloc(size);
    if (!data) {
        fprintf(stderr, "file loader: Malloc fail!\n");
        return NULL;
    }

    fread(data, size, 1, file);
    fclose(file);

    data[size - 1] = '\0';

    return data;
}


const char *bagC_font = "riglia/font.ttf";
// const char *bagC_text = "Sub zero pouches liquefying, V eat jam KEKW xd.";
const char *bagC_text = "Omega L Y L";
#define FONT_SIZE 20

static struct
{
    unsigned int vao;
    unsigned int vertex;
    unsigned int fragment;
    unsigned int program;
    unsigned int texture;
    unsigned int storage;
    struct 
    {
        int screenRes;
        int position;
        int chars;
    } uniform;
    int textSize;
    int text[400];
} global;


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
    Glyph *glyphs = malloc(fontInfo.numGlyphs * sizeof(Glyph));
    if (!bitmaps || !rects || !glyphs) {
        fprintf(stderr, "bag console: Malloc fail!\n");
        return -1;
    }

    float fontScale = stbtt_ScaleForPixelHeight(&fontInfo, FONT_SIZE);
    printf("What is it? %f\n", fontScale);
    // float fontScale = (float)FONT_SIZE;

    for (int i = 0; i < fontInfo.numGlyphs; i++) {
        int xOff, yOff, width, height;
        unsigned char *bitmap = stbtt_GetGlyphBitmap(
                &fontInfo,
                0,
                fontScale,
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
        rects[i].flags = 0;
        rects[i].index = i;
        glyphs[i].w = width;
        glyphs[i].h = height;
        glyphs[i].xOff = xOff;
        glyphs[i].yOff = yOff;
    }

    clock_t t = clock();
    packRects(rects, fontInfo.numGlyphs);
    double taken = (double)(clock() - t) / CLOCKS_PER_SEC;
    printf("Took %lf\n", taken);


    Span atlasSpan = getSpan(rects, fontInfo.numGlyphs);
    atlasSpan.w = ceil(atlasSpan.w / 4.f) * 4;  // necessary for alignment

    uint8_t *atlasBuffer = malloc(atlasSpan.w * atlasSpan.h);

    if (!atlasBuffer) {
        fprintf(stderr, "Failed malloc! Allocating glyph atlas buffer.\n");
        return -1;
    }

    memset(atlasBuffer, 0, atlasSpan.w * atlasSpan.h);

    for (int i = 0; i < fontInfo.numGlyphs; i++) {
        int bitmapIndex = rects[i].index;

        glyphs[bitmapIndex].x = rects[i].x;
        glyphs[bitmapIndex].y = rects[i].y;

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
    // free(fontBuffer);


#if 0
    int sp = 0;
    while (bagC_text[sp] != 0) {
        global.text[sp] = stbtt_FindGlyphIndex(&fontInfo, bagC_text[sp]);
        ++sp;
    }
    global.textSize = sp;
#endif


    glGenVertexArrays(1, &global.vao);
    glBindVertexArray(global.vao);



    glBindVertexArray(0);


    glGenBuffers(1, &global.storage);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, global.storage);
    glBufferData(
            GL_SHADER_STORAGE_BUFFER,
            fontInfo.numGlyphs * sizeof(Glyph),
            glyphs,
            GL_STATIC_DRAW
    );
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, global.storage);


    glGenTextures(1, &global.texture);
    glBindTexture(GL_TEXTURE_2D, global.texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            atlasSpan.w,
            atlasSpan.h,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            atlasBuffer
    );

    glBindTexture(GL_TEXTURE_2D, 0);


    int success;
    char infoLog[512];

    char *vertexSource = readFileToString("shaders/vertex.glsl");
    char *fragmentSource = readFileToString("shaders/fragment.glsl");

    global.vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(global.vertex, 1, (const char * const*)&vertexSource, NULL);
    glCompileShader(global.vertex);
    glGetShaderiv(global.vertex, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(global.vertex, 512, NULL, infoLog);
        fprintf(stderr, "Failed to compile vertex shader!\nError: %s\n", infoLog);
        return -1;
    }

    global.fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(global.fragment, 1, (const char * const*)&fragmentSource, NULL);
    glCompileShader(global.fragment);
    glGetShaderiv(global.fragment, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(global.fragment, 512, NULL, infoLog);
        fprintf(stderr, "Failed to compile fragment shader!\nError: %s\n", infoLog);
        return -1;
    }

    global.program = glCreateProgram();
    glAttachShader(global.program, global.vertex);
    glAttachShader(global.program, global.fragment);
    glLinkProgram(global.program);
    glGetProgramiv(global.program, GL_LINK_STATUS, &success);

    if (!success) {
        glGetProgramInfoLog(global.program, 512, NULL, infoLog);
        fprintf(stderr, "Failed to link shader program!\nError: %s\n", infoLog);
        return -1;
    }

    free(vertexSource);
    free(fragmentSource);

    global.uniform.screenRes = glGetUniformLocation(global.program, "u_screenRes");
    global.uniform.position  = glGetUniformLocation(global.program, "u_position");
    global.uniform.chars     = glGetUniformLocation(global.program, "u_chars");

    int index = 0, ypos;
    float xpos = 0;
    stbtt_GetFontVMetrics(&fontInfo, &ypos, 0, 0);
    ypos *= fontScale;
    int glyphIndex = stbtt_FindGlyphIndex(&fontInfo, bagC_text[0]);
    while (bagC_text[index]) {
        int advance, lsb, newGlyphIndex;
        stbtt_GetGlyphHMetrics(&fontInfo, glyphIndex, &advance, &lsb);

        global.text[index * 4]     = (int)xpos;
        global.text[index * 4 + 1] = ypos;
        global.text[index * 4 + 2] = 0xFFBFBEB5;  //b5bebf
        global.text[index * 4 + 3] = glyphIndex;

        xpos += advance * fontScale;
        if (bagC_text[index + 1]) {
            newGlyphIndex = stbtt_FindGlyphIndex(&fontInfo, bagC_text[index + 1]);
            xpos += fontScale * stbtt_GetGlyphKernAdvance(&fontInfo, glyphIndex, newGlyphIndex);
        }

        glyphIndex = newGlyphIndex;
        ++index;
    }
    global.textSize = index;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, global.texture);
    glBindVertexArray(global.vao);
    glUseProgram(global.program);

    return 0;
}

void bagC_drawCringe()
{
    int position[] = { 100, 100 };
    int screenRes[2];
    bagE_getWindowSize(screenRes, screenRes+1);

    glProgramUniform2iv(global.program, global.uniform.screenRes, 1, screenRes);
    glProgramUniform2iv(global.program, global.uniform.position, 1, position);
    glProgramUniform4iv(global.program, global.uniform.chars, global.textSize, global.text);

    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, global.textSize);

    // glBindVertexArray(0);
    // glUseProgram(0);
}

#undef FAILED_FILE_IO
