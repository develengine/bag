#include "bag_console.h"

#include <stdio.h>
#include <stdlib.h>


typedef enum
{
    bagC_WinRes,
    bagC_Position,
    bagC_Size,
    bagC_Color,

    bagC_UniformCount
} bagC_Uniform;

char const *const bagC_uniformNames[] = {
    "u_winRes",
    "u_position",
    "u_size",
    "u_color"
};


struct bagC
{
    unsigned vertShader;
    unsigned fragShader;
    unsigned program;
    unsigned vao;

    int uniforms[bagC_UniformCount];
} bagC;


char *bagC_readFile(const char *name)
{
    FILE *file = NULL;
    char *contents = NULL;

    file = fopen(name, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open file \"%s\"!\n", name);
        fprintf(stderr, "File: %s, Line: %d\n", __FILE__, __LINE__);
        goto error_exit;
    }

    if (fseek(file, 0, SEEK_END)) {
        fprintf(stderr, "Failed to seek in file \"%s\"!\n", name);
        fprintf(stderr, "File: %s, Line: %d\n", __FILE__, __LINE__);
        goto error_exit;
    }

    long size = ftell(file);
    if (size == -1L) {
        fprintf(stderr, "Failed to tell in file \"%s\"!\n", name);
        fprintf(stderr, "File: %s, Line: %d\n", __FILE__, __LINE__);
        goto error_exit;
    }

    rewind(file);

    contents = malloc(size);
    if (!contents) {
        fprintf(stderr, "Failed to allocate memory!\n");
        fprintf(stderr, "File: %s, Line: %d\n", __FILE__, __LINE__);
        goto error_exit;
    }

    if (fread(contents, 1, size, file) != (size_t)size) {
        fprintf(stderr, "Failed to read file \"%s\"!\n", name);
        fprintf(stderr, "File: %s, Line: %d\n", __FILE__, __LINE__);
        goto error_exit;
    }

    if (fclose(file)) {
        fprintf(stderr, "Failed to close file \"%s\"!\n", name);
    }
    file = NULL;

    return contents;

error_exit:
    if (file) {
        fclose(file);
    }
    free(contents);
    return NULL;
}


int bagC_loadShader(
        const char *vertexSource,
        const char *fragmentSource,
        unsigned *vertexShader,
        unsigned *fragmentShader,
        unsigned *shaderProgram)
{
    unsigned vertex = 0, fragment = 0, program = 0;
    int success;
    char infoLog[512];

    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, (const char * const*)&vertexSource, NULL);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        fprintf(stderr, "Failed to compile vertex shader!\nError: %s\n", infoLog);
        fprintf(stderr, "File: %s, Line: %d\n", __FILE__, __LINE__);
        goto error_exit;
    }

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, (const char * const*)&fragmentSource, NULL);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        fprintf(stderr, "Failed to compile fragment shader!\nError: %s\n", infoLog);
        fprintf(stderr, "File: %s, Line: %d\n", __FILE__, __LINE__);
        goto error_exit;
    }

    program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        fprintf(stderr, "Failed to link shader program!\nError: %s\n", infoLog);
        fprintf(stderr, "File: %s, Line: %d\n", __FILE__, __LINE__);
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


int bagC_init(int windowWidth, int windowHeight)
{
    char *vertSource = NULL;
    char *fragSource = NULL;

    vertSource = bagC_readFile("shaders/rect_vert.glsl");
    if (!vertSource) {
        fprintf(stderr, "bag console: Failed to read vertex shader!\n");
        goto error_exit;
    }

    fragSource = bagC_readFile("shaders/rect_frag.glsl");
    if (!fragSource) {
        fprintf(stderr, "bag console: Failed to read fragment shader!\n");
        goto error_exit;
    }

    int result = bagC_loadShader(
            vertSource, 
            fragSource, 
            &(bagC.vertShader), 
            &(bagC.fragShader), 
            &(bagC.program)
    );
    if (result) {
        fprintf(stderr, "bag console: Failed load shader!\n");
        goto error_exit;
    }

    free(vertSource);
    vertSource = NULL;
    free(fragSource);
    fragSource = NULL;

    for (bagC_Uniform u = bagC_WinRes; u < bagC_UniformCount; u++) {
        int location = glGetUniformLocation(bagC.program, bagC_uniformNames[u]);
        if (location == -1) {
            fprintf(stderr, "bag console: Failed to load uniform location for \"%s\"!\n", bagC_uniformNames[u]);
            goto error_exit;
        }
        bagC.uniforms[u] = location;
    }

    glGenVertexArrays(1, &(bagC.vao));

    bagC_updateResolution(windowWidth, windowHeight);

    return 0;

error_exit:
    free(vertSource);
    free(fragSource);
    bagC_destroy();
    return -1;
}


void bagC_render(float deltaTime)
{
    glUseProgram(bagC.program);
    glBindVertexArray(bagC.vao);

    glProgramUniform2i(bagC.program, bagC.uniforms[bagC_Position], 100, 100);
    glProgramUniform2i(bagC.program, bagC.uniforms[bagC_Size], 100, 100);
    glProgramUniform4f(bagC.program, bagC.uniforms[bagC_Color], 
            0x0d / 255.f,
            0x11 / 255.f,
            0x17 / 255.f,
            0.95f
    );

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
    glUseProgram(0);
}


void bagC_destroy()
{
    glDeleteShader(bagC.vertShader);
    glDeleteShader(bagC.fragShader);
    glDeleteProgram(bagC.program);
    glDeleteVertexArrays(1, &(bagC.vao));
}


void bagC_updateResolution(int windowWidth, int windowHeight)
{
    glProgramUniform2i(bagC.program, bagC.uniforms[bagC_WinRes], windowWidth, windowHeight);
}

