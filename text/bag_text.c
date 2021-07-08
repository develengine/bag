// FIXME: error handling when no instance bound
//        truncate when maximum simple string size exceeded
//        error when converting illegal UTF8

/* TODO
 * [X] int bagT_init(int screenWidth, int screenHeight);
 * [X] void bagT_updateResolution(int screenWidth, int screenHeight);
 * [X] void bagT_useProgram(bagT_Program program);
 * [X] void bagT_destroy();
 *
 * [X] bagT_Font *bagT_initFont(const char *path, int index);
 * [X] bagT_Instance *bagT_instantiate(bagT_Font *font, float fontSize);
 * [X] void bagT_bindInstance(bagT_Instance *instance);
 * [X] void bagT_unbindInstance();
 * [X] void bagT_destroyFont(bagT_Font *font);
 * [X] void bagT_destroyInstance(bagT_Instance *font);
 *
 * [ ] int bagT_allocateMemory(bagT_Memory **memory, int length, bagT_MemoryType type);
 * [ ] int bagT_fillMemory(bagT_Memory *memory, bagT_Char *chars, int offset, int length);
 * [ ] void bagT_bindMemory(bagT_Memory *memory);
 * [ ] int bagT_freeMemory(bagT_Memory *memory);
 *
 * [X] int bagT_codepointToGlyphIndex(bagT_Instance *instance, int codepoint);
 * [X] int bagT_UTF8ToGlyphIndex(bagT_Instance *instance, const unsigned char *ch, int *offset);
 * [X] void bagT_getOffset(bagT_Instance *instance, int glyphIndex, int *x, int *y);
 * [X] int bagT_getAdvance(bagT_Instance *instance, int glyphIndex);
 * [X] float bagT_getKerning(bagT_Instance *instance, int glyphIndex1, int glyphIndex2);
 * [X] bagT_VMetrics bagT_getVMetrics(bagT_Instance *instance);
 *
 * [X] int bagT_UTF8Length(const unsigned char *string);
 *
 * [X] void bagT_renderUTF8String(
 *             const char *string,
 *             int x, int y,
 *             bagT_Compositor compositor,
 *             void *compositorData
 *     );
 *
 * [ ] void bagT_renderCodepoints(
 *             int *codepoints,
 *             int count,
 *             int x, int y,
 *             bagT_Compositor compositor,
 *             void *compositorData
 *     );
 *
 * [X] void bagT_renderChars(bagT_Char *chars, int count, int x, int y);
 *
 * [ ] int bagT_renderMemory(int index, int count, int x, int y);
 */

#include "bag_text.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include BAGT_GL_PATH

#include <stdio.h>
#include <string.h>
#include <limits.h>

#define BAGT_FILE_ERROR()\
    fprintf(stderr, "bag text: Failed file IO!\nFile: %s, Line: %d\n", __FILE__, __LINE__)

#define BAGT_MALLOC_ERROR()\
    fprintf(stderr, "bag text: Malloc fail!\nFile: %s, Line: %d\n", __FILE__, __LINE__)


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


struct bagT_Font
{
    stbtt_fontinfo fontInfo;
    unsigned char *fontData;
};


struct bagT_Instance
{
    bagT_Font *font;
    float fontScale;
    Glyph *glyphBuffer;
    unsigned int vao;
    unsigned int atlas;
    unsigned int glyphs;
};


struct bagT_Memory
{
    int leMember;
};


typedef struct
{
    unsigned int vertexShader;
    unsigned int fragmentShader;
    unsigned int shaderProgram;

    struct {
        int screenRes;
        int position;
        int chars;
    } uni;
} bagT_Shader;


struct BagT
{
    bagT_Shader simple;
    bagT_Shader memory;

    int color;

    bagT_Instance *boundInstance;

    struct {
        int glyphIndexBuffer[BAGT_MAX_RENDER_STRING_LENGTH];
        bagT_Char charBuffer[BAGT_MAX_RENDER_STRING_LENGTH];
    } scratch;
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

    if (fread(data, 1, size, file) != size) {
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
    char *vertexSource = NULL, *fragmentSource = NULL;
    unsigned int vertex = 0, fragment = 0, program = 0;
    int success;
    char infoLog[512];

    vertexSource = bagT_readFileToString(vertexPath);
    if (!vertexSource) {
        fprintf(stderr, "bag text: Failed to load vertex shader source code!\n");
        goto error_exit;
    }

    fragmentSource = bagT_readFileToString(fragmentPath);
    if (!fragmentSource) {
        fprintf(stderr, "bag text: Failed to load fragment shader source code!\n");
        goto error_exit;
    }

    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, (const char * const*)&vertexSource, NULL);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        fprintf(stderr, "bag text: Failed to compile vertex shader!\nError: %s\n", infoLog);
        goto error_exit;
    }

    free(vertexSource);
    vertexSource = NULL;

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, (const char * const*)&fragmentSource, NULL);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        fprintf(stderr, "bag text: Failed to compile fragment shader!\nError: %s\n", infoLog);
        goto error_exit;
    }

    free(fragmentSource);
    fragmentSource = NULL;

    program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        fprintf(stderr, "bag text: Failed to link shader program!\nError: %s\n", infoLog);
        goto error_exit;
    }

    *vertexShader = vertex;
    *fragmentShader = fragment;
    *shaderProgram = program;
    return 0;

error_exit:
    free(fragmentSource);
    free(vertexSource);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    glDeleteProgram(program);
    return -1;
}


#define BAGT_UNIFORM_CHECK(uniform, name)\
    if (uniform == -1)\
        fprintf(stderr, "bag text: Failed to retrieve unifrom \"%s\" from the shader!\n"\
                        "File: %s, Line: %d\n", name, __FILE__, __LINE__);

int bagT_init(int screenWidth, int screenHeight)
{
    bagT.color = 0xffffffff;

    int result = bagT_loadShader(
            "shaders/simple_vert.glsl",
            "shaders/simple_frag.glsl",
            &bagT.simple.vertexShader,
            &bagT.simple.fragmentShader,
            &bagT.simple.shaderProgram
    );
    if (result) {
        fprintf(stderr, "bag text: Failed to load simple program!\n");
        return -1;
    }

    bagT.simple.uni.screenRes = glGetUniformLocation(bagT.simple.shaderProgram, "u_screenRes");
    BAGT_UNIFORM_CHECK(bagT.simple.uni.screenRes, "u_screenRes");
    bagT.simple.uni.position  = glGetUniformLocation(bagT.simple.shaderProgram, "u_position");
    BAGT_UNIFORM_CHECK(bagT.simple.uni.position, "u_position");
    bagT.simple.uni.chars     = glGetUniformLocation(bagT.simple.shaderProgram, "u_chars");
    BAGT_UNIFORM_CHECK(bagT.simple.uni.chars, "u_chars");

    result = bagT_loadShader(
            "shaders/memory_vert.glsl",
            "shaders/memory_frag.glsl",
            &bagT.memory.vertexShader,
            &bagT.memory.fragmentShader,
            &bagT.memory.shaderProgram
    );
    if (result) {
        fprintf(stderr, "bag text: Failed to load memory program!\n");
        bagT_destroyShader(&bagT.simple);
        return -1;
    }

    bagT.memory.uni.screenRes = glGetUniformLocation(bagT.memory.shaderProgram, "u_screenRes");
    BAGT_UNIFORM_CHECK(bagT.memory.uni.screenRes, "u_screenRes");
    bagT.memory.uni.position  = glGetUniformLocation(bagT.memory.shaderProgram, "u_position");
    BAGT_UNIFORM_CHECK(bagT.memory.uni.position, "u_position");

    bagT_updateResolution(screenWidth, screenHeight);

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
            bagT.memory.shaderProgram,
            bagT.memory.uni.screenRes,
            screenWidth, screenHeight
    );
}


void bagT_useProgram(bagT_Program program)
{
    switch (program)
    {
        case bagT_NoProgram:
            glUseProgram(0);
            break;

        case bagT_SimpleProgram:
            glUseProgram(bagT.simple.shaderProgram);
            break;

        case bagT_MemoryProgram:
            glUseProgram(bagT.memory.shaderProgram);
            break;
    }
}


void bagT_destroy()
{
    bagT_destroyShader(&bagT.simple);
    bagT_destroyShader(&bagT.memory);
}


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


bagT_Font *bagT_initFont(const char *path, int index)
{
    bagT_Font *font = malloc(sizeof(bagT_Font));
    if (!font) {
        BAGT_MALLOC_ERROR();
        goto error_exit;
    }

    font->fontData = NULL;


    /* load font file to memory */

    FILE *fontFile = fopen(path, "rb");
    if (!fontFile) {
        fprintf(stderr, "bag font: Failed to load font file!\n");
        goto error_exit;
    }

    if (fseek(fontFile, 0L, SEEK_END)) {
        BAGT_FILE_ERROR();
        goto error_exit;
    }

    int fontFileSize = ftell(fontFile);
    if (fontFileSize == -1L) {
        BAGT_FILE_ERROR();
        goto error_exit;
    }

    rewind(fontFile);

    font->fontData = malloc(fontFileSize);
    if (!font->fontData) {
        BAGT_MALLOC_ERROR();
        goto error_exit;
    }

    if (fread(font->fontData, 1, fontFileSize, fontFile) != fontFileSize) {
        BAGT_FILE_ERROR();
        goto error_exit;
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
        goto error_exit;
    }

    return font;

error_exit:
    free(font->fontData);
    free(font);
    return NULL;
}


bagT_Instance *bagT_instantiate(bagT_Font *font, float fontSize)
{
    unsigned char **bitmaps = NULL;
    Rectangle *rects = NULL;
    Glyph *glyphs = NULL;
    unsigned char *atlasBuffer = NULL;

    bagT_Instance *instance = malloc(sizeof(bagT_Instance));
    if (!instance) {
        BAGT_MALLOC_ERROR();
        goto error_exit;
    }

    instance->font = font;
    instance->fontScale = stbtt_ScaleForPixelHeight(&font->fontInfo, fontSize);

    
    /* build atlas and glyphs array */

    int glyphCount = font->fontInfo.numGlyphs;  // NOTE: """opaque type"""

    bitmaps = malloc(glyphCount * sizeof(unsigned char*));
    if (!bitmaps) {
        BAGT_MALLOC_ERROR();
        goto error_exit;
    }

    rects = malloc(glyphCount * sizeof(Rectangle));
    if (!rects) {
        BAGT_MALLOC_ERROR();
        goto error_exit;
    }

    glyphs = malloc(glyphCount * sizeof(Glyph));
    if (!glyphs) {
        BAGT_MALLOC_ERROR();
        goto error_exit;
    }

    for (int i = 0; i < glyphCount; i++) {
        int xOff, yOff, width, height;
        unsigned char *bitmap = stbtt_GetGlyphBitmap(
                &font->fontInfo,
                0,
                instance->fontScale,
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

    packRects(rects, glyphCount);

    Span atlasSpan = getSpan(rects, glyphCount);
    atlasSpan.w = ceil(atlasSpan.w / 4.f) * 4;  // necessary for alignment

    atlasBuffer = malloc(atlasSpan.w * atlasSpan.h);
    if (!atlasBuffer) {
        BAGT_MALLOC_ERROR();
        goto error_exit;
    }

    memset(atlasBuffer, 0, atlasSpan.w * atlasSpan.h);

    for (int i = 0; i < glyphCount; i++) {
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
    bitmaps = NULL;
    free(rects);
    rects = NULL;


    /* load atlas and glyphs array to opengl */

    glGenTextures(1, &instance->atlas);
    glBindTexture(GL_TEXTURE_2D, instance->atlas);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
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

    free(atlasBuffer);
    atlasBuffer = NULL;

    glGenVertexArrays(1, &instance->vao);
    glBindVertexArray(instance->vao);

    glGenBuffers(1, &instance->glyphs);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, instance->glyphs);
    glBufferData(
            GL_SHADER_STORAGE_BUFFER,
            glyphCount * sizeof(Glyph),
            glyphs,
            GL_STATIC_DRAW
    );
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindVertexArray(0);

    instance->glyphBuffer = glyphs;

    return instance;

error_exit:
    free(glyphs);
    free(rects);
    free(bitmaps);
    free(atlasBuffer);
    free(instance);
    return NULL;
}


void bagT_bindInstance(bagT_Instance *instance)
{
    glBindVertexArray(instance->vao);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, instance->glyphs);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, instance->atlas);
    bagT.boundInstance = instance;
}


void bagT_unbindInstance()
{
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    bagT.boundInstance = NULL;
}


void bagT_destroyFont(bagT_Font *font)
{
    free(font->fontData);
    free(font);
}


void bagT_destroyInstance(bagT_Instance *instance)
{
    free(instance->glyphBuffer);
    glDeleteTextures(1, &instance->atlas);
    glDeleteBuffers(1, &instance->glyphs);
    free(instance);
}


int bagT_codepointToGlyphIndex(bagT_Instance *instance, int codepoint)
{
    return stbtt_FindGlyphIndex(&(instance->font->fontInfo), codepoint);
}


static unsigned int bagT_UTF8ToUTF32(const unsigned char *s, int *offset)
{
    if ((s[0] & 0x80) == 0) {
        *offset = 1;
        return s[0];
    }

    if ((s[0] & 0xE0) == 0xC0) {
        *offset = 2;
        return ((s[0] & 0x1F) << 6) |
                (s[1] & 0x3F);
    }

    if ((s[0] & 0xF0) == 0xE0) {
        *offset = 3;
        return ((s[0] & 0x0F) << 12) |
               ((s[1] & 0x3F) << 6)  |
                (s[2] & 0x3F);
    }

    if ((s[0] & 0xF8) == 0xF0) {
        *offset = 4;
        return ((s[0] & 0x07) << 18) |
               ((s[1] & 0x3F) << 12) |
               ((s[2] & 0x3F) << 6)  |
                (s[3] & 0x3F);
    }

    *offset = 0;
    return 0;
}


static unsigned int bagT_UTF8Width(const unsigned char *s)
{
    if ((s[0] & 0x80) == 0)
        return 1;

    if ((s[0] & 0xE0) == 0xC0)
        return 2;

    if ((s[0] & 0xF0) == 0xE0)
        return 3;

    if ((s[0] & 0xF8) == 0xF0)
        return 4;

    return 0;
}


int bagT_UTF8ToGlyphIndex(bagT_Instance *instance, const unsigned char *ch, int *offset)
{
    return stbtt_FindGlyphIndex(&(instance->font->fontInfo), bagT_UTF8ToUTF32(ch, offset));
}


void bagT_getOffset(bagT_Instance *instance, int glyphIndex, int *x, int *y)
{
    *x = instance->glyphBuffer[glyphIndex].xOff;
    *y = instance->glyphBuffer[glyphIndex].yOff;
}


float bagT_getAdvance(bagT_Instance *instance, int glyphIndex)
{
    int advance, lsb;
    stbtt_GetGlyphHMetrics(&(instance->font->fontInfo), glyphIndex, &advance, &lsb);
    return advance * instance->fontScale;
}


float bagT_getKerning(bagT_Instance *instance, int glyphIndex1, int glyphIndex2)
{
    return instance->fontScale
         * stbtt_GetGlyphKernAdvance(&(instance->font->fontInfo), glyphIndex1, glyphIndex2);
}


bagT_VMetrics bagT_getVMetrics(bagT_Instance *instance)
{
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&(instance->font->fontInfo), &ascent, &descent, &lineGap);

    bagT_VMetrics metrics = {
        ascent * instance->fontScale,
        descent * instance->fontScale,
        lineGap * instance->fontScale
    };
    return metrics;
}


int bagT_UTF8Length(const unsigned char *string)
{
    int offset = 0, length = 0;
    for (; string[offset]; offset += bagT_UTF8Width(string + offset), length++);
    return length;
}


static void bagT_fallbackCompositor(void *data, bagT_Char *chars, int *glyphIndices, int length)
{
    bagT_VMetrics vMetrics = bagT_getVMetrics(bagT.boundInstance);
    float xPos = 0;

    for (int i = 0; i < length; i++) {
        int glyphIndex = glyphIndices[i];

        chars[i].x = xPos;
        chars[i].y = vMetrics.ascent;
        chars[i].color = bagT.color;
        chars[i].glyphIndex = glyphIndex;

        float advance = bagT_getAdvance(bagT.boundInstance, glyphIndex);
        xPos += advance;

        if (i < length - 1) {
            xPos += bagT_getKerning(bagT.boundInstance, glyphIndex, glyphIndices[i + 1]);
        }
    }
}


void bagT_renderUTF8String(
        const char *string,
        int x, int y,
        bagT_Compositor compositor,
        void *compositorData
) {
    int length = bagT_UTF8Length((const unsigned char*)string);

    int move;
    for (int i = 0; i < length; i++) {
        bagT.scratch.glyphIndexBuffer[i] = bagT_UTF8ToGlyphIndex(
                bagT.boundInstance,
                (const unsigned char*)string,
                &move
        );
        string += move;
    }

    if (!compositor) {
        compositor = &bagT_fallbackCompositor;
    }

    compositor(
            compositorData,
            bagT.scratch.charBuffer,
            bagT.scratch.glyphIndexBuffer, 
            length
    );

    bagT_renderChars(bagT.scratch.charBuffer, length, x, y);
}


void bagT_renderCodepoints(
        int *codepoints,
        int count,
        int x, int y,
        bagT_Compositor compositor,
        void *compositorData
) {
    for (int i = 0; i < count; i++) {
        bagT.scratch.glyphIndexBuffer[i] = bagT_codepointToGlyphIndex(
                bagT.boundInstance,
                codepoints[i]
        );
    }

    if (!compositor) {
        compositor = &bagT_fallbackCompositor;
    }

    compositor(
            compositorData,
            bagT.scratch.charBuffer,
            bagT.scratch.glyphIndexBuffer, 
            count
    );

    bagT_renderChars(bagT.scratch.charBuffer, count, x, y);
}


void bagT_renderChars(bagT_Char *chars, int count, int x, int y)
{
    glProgramUniform2i(bagT.simple.shaderProgram, bagT.simple.uni.position, x, y);
    glProgramUniform4iv(bagT.simple.shaderProgram, bagT.simple.uni.chars, count, (int*)chars);
    glUseProgram(bagT.simple.shaderProgram);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, count);
    glUseProgram(0);
}