#version 330 core

out vec4 o_color;

in vec4 v_color;
in vec4 v_uv;

uniform sampler2D u_texture;

void main() {
    o_color = v_color * texture(u_texture, v_uv.xy);
}
