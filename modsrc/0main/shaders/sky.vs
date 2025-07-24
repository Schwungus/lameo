#version 330 core

layout(location = 0) in vec3 i_position;
layout(location = 2) in vec4 i_color;
layout(location = 3) in vec4 i_uv;

out vec4 v_color;
out vec4 v_uv;

uniform mat4 u_mvp_matrix;
uniform float u_time;
uniform vec2 u_scroll;

void main() {
    gl_Position = u_mvp_matrix * vec4(i_position, 1.0);

    v_color = i_color;
    v_uv = i_uv;
    v_uv.xy += u_time * u_scroll;
}
