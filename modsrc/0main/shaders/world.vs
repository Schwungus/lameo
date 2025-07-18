#version 330 core

#define MAX_BONES 128

layout (location = 0) in vec3 i_position;
layout (location = 1) in vec3 i_normal;
layout (location = 2) in vec4 i_color;
layout (location = 3) in vec4 i_uv;
layout (location = 4) in ivec4 i_bone_index;
layout (location = 5) in vec4 i_bone_weight;

out vec4 v_color;
out vec4 v_uv;

uniform mat4 u_model_matrix;
uniform mat4 u_view_matrix;
uniform mat4 u_projection_matrix;
uniform mat4 u_mvp_matrix;

uniform bool u_animated;
uniform vec4 u_sample[2 * MAX_BONES];

void main() {
    gl_Position = u_mvp_matrix * vec4(i_position, 1.0);

    v_color = i_color;
    v_uv = i_uv;
}
