#version 430 core

const vec2 data[6] = {
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(1.0, 1.0),

    vec2(1.0, 1.0),
    vec2(0.0, 1.0),
    vec2(0.0, 0.0),
};


out V_OUT
{
    vec4 color;
} frag;


uniform ivec2 u_winRes;
uniform ivec2 u_position;
uniform ivec2 u_size;
uniform vec4 u_color;


void main()
{
    vec2 coords = data[gl_VertexID];
    vec2 pos    = ((vec2(u_position) + coords * u_size) / u_winRes) * 2.0 - 1;
    pos.y       = pos.y * -1.0;

    frag.color  = u_color;
    gl_Position = vec4(pos, 1.0, 1.0);
}
