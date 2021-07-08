#version 430 core

const int MAX_CHARS_LENGTH = 512;
const float O_1 = 0.9999;
const float O_0 = 0;

const vec2 data[6] = {
    vec2(O_0, O_0),
    vec2(O_1, O_0),
    vec2(O_1, O_1),

    vec2(O_1, O_1),
    vec2(O_0, O_1),
    vec2(O_0, O_0)
};


out V_OUT
{
    flat ivec2 origin;
    flat ivec2 size;
    vec2  move;
    vec4  color;
} frag;


uniform ivec2 u_screenRes;
uniform ivec2 u_position;
uniform ivec4 u_chars[MAX_CHARS_LENGTH];
/* .x - x coordinate
 * .y - y coordinate
 * .z - color
 * .w - glyph index
 */


struct Glyph
{
    int x;
    int y;
    int w;
    int h;
    int xOff;
    int yOff;
};

layout(std430, binding = 0) readonly buffer GlyphBuffer
{
    Glyph glyphs[];
};


void main()
{
    ivec4 ch    = u_chars[gl_InstanceID];
    Glyph glyph = glyphs[ch.w];

    ivec2 ipos  = u_position + ch.xy + ivec2(glyph.xOff, glyph.yOff);
    vec2 coord  = data[gl_VertexID];
    vec2 scoord = coord * (vec2(glyph.w, glyph.h) / u_screenRes) + (vec2(ipos) / u_screenRes);
    vec2 vcoord = vec2(scoord.x, 1.0 - scoord.y) * 2.0 - 1.0;

    frag.origin = ivec2(glyph.x, glyph.y);
    frag.size   = ivec2(glyph.w, glyph.h);
    frag.move   = coord;
    frag.color  = unpackUnorm4x8(ch.z);
    gl_Position = vec4(vcoord, 1.0, 1.0);
}

