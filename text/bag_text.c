/* FIXME
 */

/* TODO
 * [ ] support custom allocators
 */

#include "bag_text.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>

#define BAGT_FILE_ERROR()\
    fprintf(stderr, "bag text: Failed file IO!\nFile: %s, Line: %d\n", __FILE__, __LINE__)

#define BAGT_MALLOC_ERROR()\
    fprintf(stderr, "bag text: Malloc fail!\nFile: %s, Line: %d\n", __FILE__, __LINE__)


#define BAGT_MAGIC_NUMBER 0x23456341


#include "rect_pack.c"


typedef struct  // TODO pack into 16 bytes
{
    int x, y, w, h;
    int xOff, yOff;
} bagT_Glyph;

typedef struct
{
    bagT_Font *font;
    float fontScale;

    int glyphCount;
    bagT_Glyph *glyphBuffer;

    unsigned char *atlasBuffer;
    bagT_Size atlasSpan;
} bagT_InstanceData;


struct bagT_Font
{
    stbtt_fontinfo fontInfo;
    unsigned char *fontData;
};


struct bagT_Instance
{
    bagT_Font *font;
    float fontScale;
    bagT_Glyph *glyphBuffer;
    unsigned int vao;
    unsigned int atlas;
    unsigned int glyphs;
};


struct bagT_Memory
{
    bagT_Instance *instance;
    unsigned int vao;
    unsigned int memoryBuffer;
};


typedef struct
{
    unsigned int vertexShader;
    unsigned int fragmentShader;
    unsigned int shaderProgram;

    struct {
        int screenRes;
        int position;
        int scale;
        int chars;
    } uni;
} bagT_Shader;


static struct BagT
{
    bagT_Shader simple;
    bagT_Shader memory;

    bagT_Instance *boundInstance;

    struct {
        int glyphIndexBuffer[BAGT_MAX_RENDER_STRING_LENGTH];
        bagT_Char charBuffer[BAGT_MAX_RENDER_STRING_LENGTH];
    } scratch;
} bagT;


static const char * const bagT_simpleVertexSource =
    "#version 430 core\n"
    "const int MAX_CHARS_LENGTH = 512;\n"
    "const float O_1 = 0.9999;\n"
    "const float O_0 = 0;\n"
    "const vec2 data[6] = {\n"
    "    vec2(O_0, O_0),\n"
    "    vec2(O_1, O_0),\n"
    "    vec2(O_1, O_1),\n"
    "    vec2(O_1, O_1),\n"
    "    vec2(O_0, O_1),\n"
    "    vec2(O_0, O_0)\n"
    "};\n"
    "out V_OUT\n"
    "{\n"
    "    flat ivec2 origin;\n"
    "    flat ivec2 size;\n"
    "    vec2  move;\n"
    "    vec4  color;\n"
    "} frag;\n"
    "uniform ivec2 u_screenRes;\n"
    "uniform ivec2 u_position;\n"
    "uniform vec2 u_scale;\n"
    "uniform ivec4 u_chars[MAX_CHARS_LENGTH];\n"
    "struct Glyph\n"
    "{\n"
    "    int x;\n"
    "    int y;\n"
    "    int w;\n"
    "    int h;\n"
    "    int xOff;\n"
    "    int yOff;\n"
    "};\n"
    "layout(std430, binding = 0) readonly buffer GlyphBuffer\n"
    "{\n"
    "    Glyph glyphs[];\n"
    "};\n"
    "void main()\n"
    "{\n"
    "    ivec4 ch    = u_chars[gl_InstanceID];\n"
    "    Glyph glyph = glyphs[ch.w];\n"
    "    ivec2 ipos  = u_position + ivec2(ch.xy * u_scale)\n"
    "                + ivec2(glyph.xOff * u_scale.x, glyph.yOff * u_scale.y);\n"
    "    vec2 coord  = data[gl_VertexID];\n"
    "    vec2 scoord = coord * ((vec2(glyph.w, glyph.h) * u_scale) / u_screenRes)\n"
    "                + (vec2(ipos) / u_screenRes);\n"
    "    vec2 vcoord = vec2(scoord.x, 1.0 - scoord.y) * 2.0 - 1.0;\n"
    "    frag.origin = ivec2(glyph.x, glyph.y);\n"
    "    frag.size   = ivec2(glyph.w, glyph.h);\n"
    "    frag.move   = coord;\n"
    "    frag.color  = unpackUnorm4x8(ch.z);\n"
    "    gl_Position = vec4(vcoord, 1.0, 1.0);\n"
    "}\n";

static const char * const bagT_simpleFragmentSource =
    "#version 430 core\n"
    "in V_OUT\n"
    "{\n"
    "    flat ivec2 origin;\n"
    "    flat ivec2 size;\n"
    "    vec2  move;\n"
    "    vec4  color;\n"
    "} vert;\n"
    "layout(location = 0) out vec4 outColor;\n"
    "layout(binding = 0) uniform sampler2D atlas;\n"
    "void main()\n"
    "{\n"
    "    float alpha = texelFetch(atlas, vert.origin + ivec2(vert.size * vert.move), 0).r;\n"
    "    outColor = vec4(vert.color.rgb, vert.color.a * alpha);\n"
    "}\n";

static const char * const bagT_memoryVertexSource =
    "#version 430 core\n"
    "const int MAX_CHARS_LENGTH = 128;\n"
    "const float O_1 = 0.9999;\n"
    "const float O_0 = 0;\n"
    "const vec2 data[6] = {\n"
    "    vec2(O_0, O_0),\n"
    "    vec2(O_1, O_0),\n"
    "    vec2(O_1, O_1),\n"
    "    vec2(O_1, O_1),\n"
    "    vec2(O_0, O_1),\n"
    "    vec2(O_0, O_0)\n"
    "};\n"
    "layout(location = 0) in ivec4 ch;\n"
    "out V_OUT\n"
    "{\n"
    "    flat ivec2 origin;\n"
    "    flat ivec2 size;\n"
    "    vec2  move;\n"
    "    vec4  color;\n"
    "} frag;\n"
    "uniform ivec2 u_screenRes;\n"
    "uniform ivec2 u_position;\n"
    "uniform vec2 u_scale;\n"
    "struct Glyph\n"
    "{\n"
    "    int x;\n"
    "    int y;\n"
    "    int w;\n"
    "    int h;\n"
    "    int xOff;\n"
    "    int yOff;\n"
    "};\n"
    "layout(std430, binding = 0) readonly buffer GlyphBuffer\n"
    "{\n"
    "    Glyph glyphs[];\n"
    "};\n"
    "void main()\n"
    "{\n"
    "    Glyph glyph = glyphs[ch.w];\n"
    "    ivec2 ipos  = u_position + ivec2(ch.xy * u_scale)\n"
    "                + ivec2(glyph.xOff * u_scale.x, glyph.yOff * u_scale.y);\n"
    "    vec2 coord  = data[gl_VertexID];\n"
    "    vec2 scoord = coord * ((vec2(glyph.w, glyph.h) * u_scale) / u_screenRes)\n"
    "                + (vec2(ipos) / u_screenRes);\n"
    "    vec2 vcoord = vec2(scoord.x, 1.0 - scoord.y) * 2.0 - 1.0;\n"
    "    frag.origin = ivec2(glyph.x, glyph.y);\n"
    "    frag.size   = ivec2(glyph.w, glyph.h);\n"
    "    frag.move   = coord;\n"
    "    frag.color  = unpackUnorm4x8(ch.z);\n"
    "    gl_Position = vec4(vcoord, 1.0, 1.0);\n"
    "}\n";

static const char * const bagT_memoryFragmentSource =
    "#version 430 core\n"
    "in V_OUT\n"
    "{\n"
    "    flat ivec2 origin;\n"
    "    flat ivec2 size;\n"
    "    vec2  move;\n"
    "    vec4  color;\n"
    "} vert;\n"
    "layout(location = 0) out vec4 outColor;\n"
    "layout(binding = 0) uniform sampler2D atlas;\n"
    "void main()\n"
    "{\n"
    "    float alpha = texelFetch(atlas, vert.origin + ivec2(vert.size * vert.move), 0).r;\n"
    "    outColor = vec4(vert.color.rgb, vert.color.a * alpha);\n"
    "}\n";


static void bagT_destroyShader(bagT_Shader *shader)
{
    glDeleteShader(shader->vertexShader);
    glDeleteShader(shader->fragmentShader);
    glDeleteProgram(shader->shaderProgram);
}


static int bagT_loadShader(
        const char *vertexSource,
        const char *fragmentSource,
        unsigned int *vertexShader,
        unsigned int *fragmentShader,
        unsigned int *shaderProgram)
{
    unsigned int vertex = 0, fragment = 0, program = 0;
    int success;
    char infoLog[512];

    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, (const char * const*)&vertexSource, NULL);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        fprintf(stderr, "bag text: Failed to compile vertex shader!\nError: %s\n", infoLog);
        goto error_exit;
    }

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, (const char * const*)&fragmentSource, NULL);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        fprintf(stderr, "bag text: Failed to compile fragment shader!\nError: %s\n", infoLog);
        goto error_exit;
    }

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
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    glDeleteProgram(program);
    return -1;
}


#define BAGT_UNIFORM_CHECK(uniform, name)\
    if (uniform == -1)\
        fprintf(stderr, "bag text: Failed to retrieve unifrom \"%s\" from the shader!\n"\
                        "File: %s, Line: %d\n", name, __FILE__, __LINE__);

int bagT_init(int windowWidth, int windowHeight)
{
    bagT.boundInstance = NULL;

    int result = bagT_loadShader(
            bagT_simpleVertexSource,
            bagT_simpleFragmentSource,
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
    bagT.simple.uni.scale     = glGetUniformLocation(bagT.simple.shaderProgram, "u_scale");
    BAGT_UNIFORM_CHECK(bagT.simple.uni.scale, "u_scale");

    result = bagT_loadShader(
            bagT_memoryVertexSource,
            bagT_memoryFragmentSource,
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
    bagT.memory.uni.scale     = glGetUniformLocation(bagT.memory.shaderProgram, "u_scale");
    BAGT_UNIFORM_CHECK(bagT.memory.uni.scale, "u_scale");

    bagT_updateResolution(windowWidth, windowHeight);

    return 0;
}


void bagT_updateResolution(int windowWidth, int windowHeight)
{
    glProgramUniform2i(
            bagT.simple.shaderProgram,
            bagT.simple.uni.screenRes,
            windowWidth, windowHeight
    );
    glProgramUniform2i(
            bagT.memory.shaderProgram,
            bagT.memory.uni.screenRes,
            windowWidth, windowHeight
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


static inline int bagT_packColor(bagT_Color color)
{
    return ((int)(color.a * 255) << 24)
         | ((int)(color.b * 255) << 16)
         | ((int)(color.g * 255) << 8)
         |  (int)(color.r * 255);
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
        fprintf(stderr, "bag text: Failed to load font file!\n");
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

    if (fclose(fontFile)) {
        BAGT_FILE_ERROR();
    }


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
    if (font) {
        free(font->fontData);
    }
    free(font);
    return NULL;
}


static int bagT_createInstanceData(bagT_InstanceData *data, bagT_Font *font, float fontSize, int fastPack)
{
    unsigned char **bitmaps = NULL;
    bagT_Rectangle *rects = NULL;
    bagT_Glyph *glyphs = NULL;
    unsigned char *atlasBuffer = NULL;

    float fontScale = stbtt_ScaleForPixelHeight(&font->fontInfo, fontSize);
    
    /* build atlas and glyphs array */

    int glyphCount = font->fontInfo.numGlyphs;  // NOTE: """opaque type"""

    bitmaps = malloc(glyphCount * sizeof(unsigned char*));
    if (!bitmaps) {
        BAGT_MALLOC_ERROR();
        goto error_exit;
    }

    rects = malloc(glyphCount * sizeof(bagT_Rectangle));
    if (!rects) {
        BAGT_MALLOC_ERROR();
        goto error_exit;
    }

    glyphs = malloc(glyphCount * sizeof(bagT_Glyph));
    if (!glyphs) {
        BAGT_MALLOC_ERROR();
        goto error_exit;
    }

    for (int i = 0; i < glyphCount; i++) {
        int xOff, yOff, width, height;
        unsigned char *bitmap = stbtt_GetGlyphBitmap(
                &font->fontInfo,
                0,
                fontScale,
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


    if (fastPack) {
        bagT_fastPackRectangles(rects, glyphCount);
    } else {
        bagT_packRectangles(rects, glyphCount);
    }

    bagT_Size atlasSpan = getSize(rects, glyphCount);
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

    data->font = font;
    data->fontScale = fontScale;
    data->glyphCount = glyphCount;
    data->glyphBuffer = glyphs;
    data->atlasBuffer = atlasBuffer;
    data->atlasSpan = atlasSpan;

    return 0;

error_exit:
    free(glyphs);
    free(rects);
    free(bitmaps);
    free(atlasBuffer);
    return 1;
}


static bagT_Instance *bagT_makeInstance(bagT_InstanceData data)
{
    bagT_Instance *instance = malloc(sizeof(bagT_Instance));
    if (!instance) {
        BAGT_MALLOC_ERROR();
        goto error_exit;
    }

    instance->font = data.font;
    instance->fontScale = data.fontScale;

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
            data.atlasSpan.w,
            data.atlasSpan.h,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            data.atlasBuffer
    );
    glBindTexture(GL_TEXTURE_2D, 0);

    free(data.atlasBuffer);
    data.atlasBuffer = NULL;

    glGenVertexArrays(1, &instance->vao);
    glBindVertexArray(instance->vao);

    glGenBuffers(1, &instance->glyphs);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, instance->glyphs);
    glBufferData(
            GL_SHADER_STORAGE_BUFFER,
            data.glyphCount * sizeof(bagT_Glyph),
            data.glyphBuffer,
            GL_STATIC_DRAW
    );

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindVertexArray(0);

    instance->glyphBuffer = data.glyphBuffer;

    return instance;

error_exit:
    free(data.atlasBuffer);
    free(data.glyphBuffer);
    free(instance);
    return NULL;
}


bagT_Instance *bagT_instantiate(bagT_Font *font, float fontSize)
{
    bagT_InstanceData data;

    if (bagT_createInstanceData(&data, font, fontSize, 1)) {
        return NULL;
    }

    return bagT_makeInstance(data);
}


#define BAGT_WRITE(ptr, size, count, file)                                                      \
    if (fwrite(ptr, size, count, file) != count) {                                              \
        fprintf(stderr, "bag text: failed to write!\nFile: %s, Line: %d\n", __FILE__, __LINE__);\
        goto error_exit;                                                                        \
    }

int bagT_createInstanceFile(bagT_Font *font, float fontSize, const char *fileName)
{
    bagT_InstanceData data = { 0 };

    if (bagT_createInstanceData(&data, font, fontSize, 0)) {
        goto error_exit;
    }

    FILE *file = fopen(fileName, "wb");
    if (!file) {
        BAGT_FILE_ERROR();
        goto error_exit;
    }

    uint32_t magic = BAGT_MAGIC_NUMBER;
    uint32_t atlasSize = data.atlasSpan.w * data.atlasSpan.h;

    BAGT_WRITE(&magic, sizeof(uint32_t), 1, file);
    BAGT_WRITE(&(data.fontScale), sizeof(float), 1, file);
    BAGT_WRITE(&(data.glyphCount), sizeof(int), 1, file);
    BAGT_WRITE(&(data.atlasSpan.w), sizeof(int), 1, file);
    BAGT_WRITE(&(data.atlasSpan.h), sizeof(int), 1, file);
    BAGT_WRITE(data.glyphBuffer, sizeof(bagT_Glyph), data.glyphCount, file);
    BAGT_WRITE(data.atlasBuffer, sizeof(unsigned char), atlasSize, file);

    fclose(file);

    return 0;

error_exit:
    free(data.atlasBuffer);
    free(data.glyphBuffer);
    if (file) {
        fclose(file);
    }
    return -1;
}


#define BAGT_READ(ptr, size, count, file)                                                       \
    if (fread(ptr, size, count, file) != count) {                                               \
        fprintf(stderr, "bag text: failed to read!\nFile: %s, Line: %d\n", __FILE__, __LINE__); \
        goto error_exit;                                                                        \
    }

bagT_Instance *bagT_loadInstanceFile(bagT_Font *font, const char *fileName)
{
    bagT_InstanceData data = { 0 };

    data.font = font;

    FILE *file = fopen(fileName, "rb");
    if (!file) {
        BAGT_FILE_ERROR();
        goto error_exit;
    }

    uint32_t magic;
    BAGT_READ(&magic, sizeof(uint32_t), 1, file);
    if (magic != BAGT_MAGIC_NUMBER) {
        fprintf(stderr, "bag text: corrupted instance file! '%s'\n", fileName);
        goto error_exit;
    }

    BAGT_READ(&(data.fontScale), sizeof(float), 1, file);
    BAGT_READ(&(data.glyphCount), sizeof(int), 1, file);
    BAGT_READ(&(data.atlasSpan.w), sizeof(int), 1, file);
    BAGT_READ(&(data.atlasSpan.h), sizeof(int), 1, file);

    data.glyphBuffer = malloc(data.glyphCount * sizeof(bagT_Glyph));
    if (!data.glyphCount) {
        BAGT_MALLOC_ERROR();
        goto error_exit;
    }
    BAGT_READ(data.glyphBuffer, sizeof(bagT_Glyph), data.glyphCount, file);

    int atlasSize = data.atlasSpan.w * data.atlasSpan.h;
    data.atlasBuffer = malloc(atlasSize * sizeof(unsigned char));
    if (!data.atlasBuffer) {
        BAGT_MALLOC_ERROR();
        goto error_exit;
    }
    BAGT_READ(data.atlasBuffer, sizeof(unsigned char), atlasSize, file);

    fclose(file);

    return bagT_makeInstance(data);

error_exit:
    free(data.atlasBuffer);
    free(data.glyphBuffer);
    if (file) {
        fclose(file);
    }
    return NULL;
}


void bagT_bindInstance(bagT_Instance *instance)
{
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
    if (!font) {
        return;
    }
    free(font->fontData);
    free(font);
}


void bagT_destroyInstance(bagT_Instance *instance)
{
    if (!instance) {
        return;
    }
    free(instance->glyphBuffer);
    glDeleteTextures(1, &instance->atlas);
    glDeleteBuffers(1, &instance->glyphs);
    glDeleteVertexArrays(1, &instance->vao);
    free(instance);
}


static unsigned int bagT_memoryTypeToDraw(bagT_MemoryType type)
{
    switch (type) {
        case bagT_StaticMemory:
            return GL_STATIC_DRAW;
        case bagT_DynamicMemory:
            return GL_DYNAMIC_DRAW;
        case bagT_StreamMemory:
            return GL_STREAM_DRAW;
        default:
            return 'S'|'W'|'A'|'G';
    }
}


bagT_Memory *bagT_allocateMemory(
        bagT_Instance *instance,
        bagT_Char *data,
        int count,
        bagT_MemoryType type
) {
    bagT_Memory *memory = malloc(sizeof(bagT_Memory));
    if (!memory) {
        BAGT_MALLOC_ERROR();
        goto error_exit;
    }

    unsigned int vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    unsigned int memoryBuffer;
    glGenBuffers(1, &memoryBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, memoryBuffer);
    glBufferData(GL_ARRAY_BUFFER, count * sizeof(bagT_Char), (void *)data, bagT_memoryTypeToDraw(type));

    glVertexAttribIPointer(0, sizeof(bagT_Char) / sizeof(int), GL_INT, sizeof(bagT_Char), (void*)0);
    glVertexAttribDivisor(0, 1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    memory->instance = instance;
    memory->vao = vao;
    memory->memoryBuffer = memoryBuffer;

    return memory;

error_exit:
    return NULL;
}


void bagT_openMemory(bagT_Memory *memory)
{
    glBindBuffer(GL_ARRAY_BUFFER, memory->memoryBuffer);
}


void bagT_fillMemory(bagT_Char *chars, int offset, int length)
{
    glBufferSubData(GL_ARRAY_BUFFER, offset * sizeof(bagT_Char), length * sizeof(bagT_Char), chars);
}


void bagT_closeMemory()
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}


void bagT_bindMemory(bagT_Memory *memory)
{
    glBindVertexArray(memory->vao);
    glEnableVertexAttribArray(0);
}


void bagT_unbindMemory()
{
    glDisableVertexAttribArray(0);
    glBindVertexArray(0);
}


void bagT_freeMemory(bagT_Memory *memory)
{
    if (!memory) {
        return;
    }
    glDeleteBuffers(1, &memory->memoryBuffer);
    glDeleteVertexArrays(1, &memory->vao);
    free(memory);
}


int bagT_codepointToGlyphIndex(bagT_Instance *instance, int codepoint)
{
    return stbtt_FindGlyphIndex(&(instance->font->fontInfo), codepoint);
}


static unsigned int bagT_UTF8ToUTF32(const char *s, int *offset)
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


static unsigned int bagT_UTF8Width(const char *s)
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


int bagT_UTF8ToGlyphIndex(bagT_Instance *instance, const char *ch, int *offset)
{
    int codepoint = bagT_UTF8ToUTF32(ch, offset);
    if (!(*offset)) {
        fprintf(stderr, "bag text: Illegal UTF8 byte '%d'!\n", ch[0]);
        *offset = 1;
    }

    return stbtt_FindGlyphIndex(&(instance->font->fontInfo), codepoint);
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


int bagT_UTF8Length(const char *string)
{
    int offset = 0, length = 0;
    for (; string[offset]; offset += bagT_UTF8Width(string + offset), length++);
    return length;
}


void bagT_simpleCompositor(
        bagT_Instance *instance, 
        void *data,
        bagT_Char *chars,
        int *glyphIndices,
        int length
) {
    bagT_VMetrics vMetrics = bagT_getVMetrics(instance);
    float xPos = 0;
    int color = 0xffffffff;

    if (data) {
        color = bagT_packColor(*((bagT_Color*)data));
    }

    for (int i = 0; i < length; i++) {
        int glyphIndex = glyphIndices[i];

        chars[i].x = (int)xPos;
        chars[i].y = vMetrics.ascent;
        chars[i].color = color;
        chars[i].glyphIndex = glyphIndex;

        float advance = bagT_getAdvance(instance, glyphIndex);
        xPos += advance;

        if (i > 0) {
            xPos += bagT_getKerning(instance, glyphIndex, glyphIndices[i + 1]);
        }
    }
}


void bagT_renderUTF8String(
        const char *string,
        bagT_Transform transform,
        bagT_Compositor compositor,
        void *compositorData
) {
    if (!bagT.boundInstance) {
        fprintf(stderr, "bag text: Rendering without binding an instance!\n");
        return;
    }

    int length = bagT_UTF8Length(string);

    if (length > BAGT_MAX_RENDER_STRING_LENGTH) {
        length = BAGT_MAX_RENDER_STRING_LENGTH;
    }

    int move;
    for (int i = 0; i < length; i++) {
        bagT.scratch.glyphIndexBuffer[i] = bagT_UTF8ToGlyphIndex(
                bagT.boundInstance,
                string,
                &move
        );
        string += move;
    }

    if (!compositor) {
        compositor = &bagT_simpleCompositor;
    }

    compositor(
            bagT.boundInstance,
            compositorData,
            bagT.scratch.charBuffer,
            bagT.scratch.glyphIndexBuffer, 
            length
    );

    bagT_renderChars(bagT.scratch.charBuffer, length, transform);
}


void bagT_renderCodepoints(
        int *codepoints,
        int count,
        bagT_Transform transform,
        bagT_Compositor compositor,
        void *compositorData
) {
    if (!bagT.boundInstance) {
        fprintf(stderr, "bag text: Rendering without binding an instance!\n");
        return;
    }

    if (count > BAGT_MAX_RENDER_STRING_LENGTH) {
        count = BAGT_MAX_RENDER_STRING_LENGTH;
    }

    for (int i = 0; i < count; i++) {
        bagT.scratch.glyphIndexBuffer[i] = bagT_codepointToGlyphIndex(
                bagT.boundInstance,
                codepoints[i]
        );
    }

    if (!compositor) {
        compositor = &bagT_simpleCompositor;
    }

    compositor(
            bagT.boundInstance,
            compositorData,
            bagT.scratch.charBuffer,
            bagT.scratch.glyphIndexBuffer, 
            count
    );

    bagT_renderChars(bagT.scratch.charBuffer, count, transform);
}


void bagT_renderChars(bagT_Char *chars, int count, bagT_Transform transform)
{
    if (!bagT.boundInstance) {
        fprintf(stderr, "bag text: Rendering without binding an instance!\n");
        return;
    }

    glBindVertexArray(bagT.boundInstance->vao);
    glProgramUniform2i(bagT.simple.shaderProgram, bagT.simple.uni.position, transform.x, transform.y);
    glProgramUniform2f(bagT.simple.shaderProgram, bagT.simple.uni.scale, transform.w, transform.h);
    glProgramUniform4iv(bagT.simple.shaderProgram, bagT.simple.uni.chars, count, (int*)chars);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, count);
    glBindVertexArray(0);
}


void bagT_renderMemory(int index, int count, bagT_Transform transform)
{
    glProgramUniform2i(bagT.memory.shaderProgram, bagT.memory.uni.position, transform.x, transform.y);
    glProgramUniform2f(bagT.memory.shaderProgram, bagT.memory.uni.scale, transform.w, transform.h);
    glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 6, count, index);
}


void bagT_multiRenderMemory(bagT_Transform transform, bagT_Segment *segments, int count)
{
    glProgramUniform2i(bagT.memory.shaderProgram, bagT.memory.uni.position, transform.x, transform.y);
    glProgramUniform2f(bagT.memory.shaderProgram, bagT.memory.uni.scale, transform.w, transform.h);
    glMultiDrawArraysIndirect(GL_TRIANGLES, segments, count, sizeof(bagT_Segment));
}
