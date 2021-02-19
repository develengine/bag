#include <stdio.h>
#include <limits.h>
#include <stdint.h>

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
    int index;
} Rectangle;

#include "rect_tile.c"

typedef struct
{
    float x, y, w, h;
    int xOff, yOff;
} GlyphUniform;

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


const char *bagC_font = "JetBrainsMono-Regular.ttf";


struct
{
    unsigned int vao;
    unsigned int vertex;
    unsigned int fragment;
    unsigned int program;
    struct 
    {
        int position;
        int scale;
    } uniform;
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
    GlyphUniform *uniforms = malloc(fontInfo.numGlyphs * sizeof(GlyphUniform));
    if (!bitmaps || !rects || !uniforms) {
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
        uniforms[i].xOff = xOff;
        uniforms[i].yOff = yOff;
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
    // free(fontBuffer);


    glGenVertexArrays(1, &global.vao);
    glBindVertexArray(global.vao);



    glBindVertexArray(0);


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
    }

    global.fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(global.fragment, 1, (const char * const*)&fragmentSource, NULL);
    glCompileShader(global.fragment);
    glGetShaderiv(global.fragment, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(global.fragment, 512, NULL, infoLog);
        fprintf(stderr, "Failed to compile fragment shader!\nError: %s\n", infoLog);
    }

    global.program = glCreateProgram();
    glAttachShader(global.program, global.vertex);
    glAttachShader(global.program, global.fragment);
    glLinkProgram(global.program);
    glGetProgramiv(global.program, GL_LINK_STATUS, &success);

    if (!success) {
        glGetProgramInfoLog(global.program, 512, NULL, infoLog);
        fprintf(stderr, "Failed to link shader program!\nError: %s\n", infoLog);
    }

    free(vertexSource);
    free(fragmentSource);

    global.uniform.position = glGetUniformLocation(global.program, "UNIFORM.position");
    global.uniform.scale = glGetUniformLocation(global.program, "UNIFORM.scale");

    return 0;
}


void bagC_drawCringe()
{
    glBindVertexArray(global.vao);
    glUseProgram(global.program);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
    glUseProgram(0);
}

#undef FAILED_FILE_IO
