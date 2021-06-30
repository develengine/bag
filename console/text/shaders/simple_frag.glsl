#version 430 core

in V_OUT
{
    flat ivec2 origin;
    flat ivec2 size;
    vec2  move;
    vec4  color;
} vert;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D atlas;

void main()
{
    float alpha = texelFetch(atlas, vert.origin + ivec2(vert.size * vert.move), 0).r;
    outColor = vec4(vert.color.rgb, vert.color.a * alpha);
}
