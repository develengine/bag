#version 430 core

in V_OUT
{
    vec4 color;
} vert;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vert.color;
}
