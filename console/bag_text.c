#include "bag_text.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

struct bagT_Font
{
    stbtt_fontinfo fontInfo;
    unsigned char *fontData;
    float fontScale;

    unsigned int atlas;
    unsigned int glyphs;
};
typedef struct bagT_Font bagT_Font;

struct bagT_Memory
{

};
typedef struct bagT_Memory bagT_Memory;

#include <stdio.h>
#include <string.h>

typedef struct
{
    int x, y;
    int w, h;
    int flags;
    int index;
} Rectangle;

#include "rect_tile.c"  // TODO make part of this file

/* TODO
 * [ ] support custom allocators
 */

typedef struct  // TODO pack into 16 bytes
{
    int x, y, w, h;
    int xOff, yOff;
} Glyph;

#define BAGT_FILE_ERROR()\
    fprintf(stderr, "bag text: Failed file IO!\nFile: %s, Line: %d\n", __FILE__, __LINE__)

#define BAGT_MALLOC_ERROR()\
    fprintf(stderr, "bag text: Malloc fail!\nFile: %s, Line: %d\n", __FILE__, __LINE__)

struct BagT
{
    struct bagT_Shader {
        unsigned int vertexShader;
        unsigned int fragmentShader;
        unsigned int shaderProgram;

        struct {
            int screenRes;
            int position;
            int chars;
        } uni;
    } simple;

    struct bagT_Shader {
        unsigned int vertexShader;
        unsigned int fragmentShader;
        unsigned int shaderProgram;

        struct {
            int screenRes;
            int position;
        } uni;
    } normal;
} bagT;


static void bagT_destroyShader(bagT_Shader *shader)
{
    glDeleteShader(shader->vertexShader);
    glDeleteShader(shader->fragmentShader);
    glDeleteProgram(shader->shaderProgram);
}


static char *bagT_readFileToString(const char *path)
{
    FILE *file = fopen(path, "r");
    
    if (!file)
        return NULL;

    if (fseek(file, 0, SEEK_END)) {
        BAGT_FILE_ERROR();
        return NULL;
    }

    int size = ftell(file);
    if (size == -1L) {
        BAGT_FILE_ERROR();
        return NULL;
    }

    rewind(file);
    
    char *data = malloc(size);
    if (!data) {
        BAGT_MALLOC_ERROR();
        return NULL;
    }

    if (fread(data, size, 1, file) != size) {
        BAGT_FILE_ERROR();
        free(data);
        return NULL;
    }

    if (fclose(file))
        BAGT_FILE_ERROR();

    data[size - 1] = '\0';

    return data;
}


static int bagT_loadShader(
        const char *vertexPath,
        const char *fragmentPath,
        unsigned int *vertexShader,
        unsigned int *fragmentShader,
        unsigned int *shaderProgram)
{
    unsigned int vertex, fragment, program;
    int success;
    char infoLog[512];

    char *vertexSource = bagT_readFileToString(vertexPath);
    if (!vertexSource) {
        fprintf(stderr, "bag text: Failed to load vertex shader source code!\n");
        return -1;
    }

    char *fragmentSource = bagT_readFileToString(fragmentPath);
    if (!fragmentSource) {
        fprintf(stderr, "bag text: Failed to load fragment shader source code!\n");
        free(vertexSource);
        return -1;
    }

    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, (const char * const*)&vertexSource, NULL);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        fprintf(stderr, "bag text: Failed to compile vertex shader!\nError: %s\n", infoLog);
        goto delete_vertex;
    }

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, (const char * const*)&fragmentSource, NULL);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        fprintf(stderr, "bag text: Failed to compile fragment shader!\nError: %s\n", infoLog);
        goto delete_fragment;
    }

    program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        fprintf(stderr, "bag text: Failed to link shader program!\nError: %s\n", infoLog);
        goto delete_program;
    }

    free(vertexSource);
    free(fragmentSource);

    *vertexShader = vertex;
    *fragmentShader = fragment;
    *shaderProgram = program;
    return 0;

delete_program:
    glDeleteShader(fragment);
delete_fragment:
    glDeleteShader(vertex);
delete_vertex:
    free(fragmentSource);
    free(vertexSource);
    return -1;
}


#define BAGT_UNIFORM_CHECK(uniform, name)\
    if (uniform == -1)\
        fprintf(stderr, "bag text: Failed to retrieve unifrom \"%s\" from the shader!\n"\
                        "File: %s, Line: %d\n", name, __FILE__, __LINE__);

int bagT_init(int screenWidth, int screenHeight)
{
    int result = bagT_loadShader(
            "shaders/simple_vert.glsl",
            "shaders/simple_frag.glsl",
            &bagT.simple.vertexShader,
            &bagT.simple.fragmentShader,
            &bagT.simple.shaderProgram
    );
    if (result) {
        fprintf(stderr, "bag text: Failed to load simple shader!\n");
        return -1;
    }

    bagT.simple.uni.screenRes = glGetUniformLocation(bagT.simple.shaderProgram, "u_screenRes");
    BAGT_UNIFORM_CHECK(bagT.simple.uni.screenRes, "u_screenRes");
    bagT.simple.uni.position  = glGetUniformLocation(bagT.simple.shaderProgram, "u_position");
    BAGT_UNIFORM_CHECK(bagT.simple.uni.position, "u_position");
    bagT.simple.uni.chars     = glGetUniformLocation(bagT.simple.shaderProgram, "u_chars");
    BAGT_UNIFORM_CHECK(bagT.simple.uni.chars, "u_chars");

    glProgramUniform2i(
            bagT.simple.shaderProgram,
            bagT.simple.uni.screenRes,
            screenWidth, screenHeight
    );

    result = bagT_loadShader(
            "shaders/normal_vert.glsl",
            "shaders/normal_frag.glsl",
            &bagT.normal.vertexShader,
            &bagT.normal.fragmentShader,
            &bagT.normal.shaderProgram
    );
    if (result) {
        fprintf(stderr, "bag text: Failed to load normal shader!\n");
        bagT_destroyShader(&bagT.simple);
        return -1;
    }

    bagT.normal.uni.screenRes = glGetUniformLocation(bagT.normal.shaderProgram, "u_screenRes");
    BAGT_UNIFORM_CHECK(bagT.normal.uni.screenRes, "u_screenRes");
    bagT.normal.uni.position  = glGetUniformLocation(bagT.normal.shaderProgram, "u_position");
    BAGT_UNIFORM_CHECK(bagT.normal.uni.position, "u_position");

    glProgramUniform2i(
            bagT.normal.shaderProgram,
            bagT.normal.uni.screenRes,
            screenWidth, screenHeight
    );

    return 0;
}


void bagT_updateResolution(int screenWidth, int screenHeight)
{
    glProgramUniform2i(
            bagT.simple.shaderProgram,
            bagT.simple.uni.screenRes,
            screenWidth, screenHeight
    );
    glProgramUniform2i(
            bagT.normal.shaderProgram,
            bagT.normal.uni.screenRes,
            screenWidth, screenHeight
    );
}


void bagT_destroy()
{
    bagT_destroyShader(&bagT.simple);
    bagT_destroyShader(&bagT.normal);
{


static inline void bagT_writeBitmap(
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


int bagT_initFont(bagT_Font *font, const char *path, float fontSize, int index)
{
    /* load font file to memory */

    FILE *fontFile = fopen(path, "rb");
    if (!fontFile) {
        fprintf(stderr, "bag font: Failed to load font file!\n");
        return -1;
    }

    if (fseek(fontFile, 0L, SEEK_END)) {
        BAGT_FILE_ERROR();
        return -1;
    }

    int fontFileSize = ftell(fontFile);
    if (fontFileSize == -1L) {
        BAGT_FILE_ERROR();
        return -1;
    }

    rewind(fontFile);

    font->fontData = malloc(fontFileSize);
    if (!font->fontData) {
        BAGT_MALLOC_ERROR();
        return -1;
    }

    if (fread(font->fontData, 1, fontFileSize, fontFile) != fontFileSize) {
        BAGT_FILE_ERROR();
        goto free_font;
    }

    if (fclose(fontFile))
        BAGT_FILE_ERROR();


    /* initialize stb_truetype */

    int result = stbtt_InitFont(
            &font->fontInfo,
            font->fontData,
            stbtt_GetFontOffsetForIndex(font->fontData, 0)
    );
    if (!result) {
        fprintf(stderr, "bag text: Failed to load font from font file!\n");
        goto free_font;
    }

    font->fontScale = stbtt_ScaleForPixelHeight(&font->fontInfo, fontSize);

    
    /* build atlas and glyphs array */

    unsigned char **bitmaps = malloc(font->fontInfo.numGlyphs * sizeof(unsigned char*));
    if (!bitmaps) {
        BAGT_MALLOC_ERROR();
        goto free_font;
    }

    Rectangle *rects = malloc(font->fontInfo.numGlyphs * sizeof(Rectangle));
    if (!rects) {
        BAGT_MALLOC_ERROR();
        goto free_bitmaps;
    }

    Glyph *glyphs = malloc(font->fontInfo.numGlyphs * sizeof(Glyph));
    if (!glyphs) {
        BAGT_MALLOC_ERROR();
        goto free_rects;
    }

    for (int i = 0; i < font->fontInfo.numGlyphs; i++) {
        int xOff, yOff, width, height;
        unsigned char *bitmap = stbtt_GetGlyphBitmap(
                &font->fontInfo,
                0,
                font->fontScale,
                i,
                &width, &height,
                &xOff, &yOff
        );

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

    packRects(rects, font->fontInfo.numGlyphs);

    Span atlasSpan = getSpan(rects, font->fontInfo.numGlyphs);
    atlasSpan.w = ceil(atlasSpan.w / 4.f) * 4;  // necessary for alignment
    uint8_t *atlasBuffer = malloc(atlasSpan.w * atlasSpan.h);
    if (!atlasBuffer) {
        BAGT_MALLOC_ERROR();
        goto free_glyphs;
    }
    memset(atlasBuffer, 0, atlasSpan.w * atlasSpan.h);

    for (int i = 0; i < fontInfo.numGlyphs; i++) {
        int index = rects[i].index;

        glyphs[index].x = rects[i].x;
        glyphs[index].y = rects[i].y;

        bagT_writeBitmap(
                bitmaps[index], atlasBuffer,
                rects[i].w, rects[i].h,
                atlasSpan.w, atlasSpan.h,
                rects[i].x, rects[i].y
        );

        stbtt_FreeBitmap(bitmaps[index], NULL);
    }
    
    free(bitmaps);
    free(rects);


    /* load atlas and glyphs array to opengl */

    glGenTextures(1, &font->atlas);
    glBindTexture(GL_TEXTURE_2D, font->atlas);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
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
    free(atlasBuffer);

    glGenBuffers(1, &font->glyphs);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, font->glyphs);
    glBufferData(
            GL_SHADER_STORAGE_BUFFER,
            font->fontInfo.numGlyphs * sizeof(Glyph),
            glyphs,
            GL_STATIC_DRAW
    );
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    free(glyphs);

    return 0;

free_glyphs:
    free(glyphs);
free_rects:
    free(rects);
free_bitmaps:
    free(bitmaps);
free_font:
    free(font->fontData);
    return -1;
}


void bagT_destroyFont(bagT_Font *font)
{
    free(font->fontData);
    glDeleteTextures(1, &font->atlas);
    glDeleteBuffers(1, &font->glyphs);
}

