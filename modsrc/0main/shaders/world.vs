#version 330 core

#define MAX_BONES 128

layout(location = 0) in vec3 i_position;
layout(location = 1) in vec3 i_normal;
layout(location = 2) in vec4 i_color;
layout(location = 3) in vec4 i_uv;
layout(location = 4) in vec4 i_bone_index;
layout(location = 5) in vec4 i_bone_weight;

out vec4 v_color;
out vec4 v_uv;

uniform mat4 u_model_matrix;
uniform mat4 u_view_matrix;
uniform mat4 u_projection_matrix;
uniform mat4 u_mvp_matrix;

uniform bool u_animated;
uniform vec4 u_sample[2 * MAX_BONES];

vec3 quat_rotate(vec4 q, vec3 v) {
	vec3 u = q.xyz;
	return (v + 2.0 * cross(u, cross(u, v) + q.w * v));
}

vec3 dq_transform(vec4 real, vec4 dual, vec3 v) {
	vec3 r3 = real.xyz;
	vec3 d3 = dual.xyz;
	return (quat_rotate(real, v) + 2.0 * (real.w * d3 - dual.w * r3 + cross(r3, d3)));
}

void main() {
    vec3 position = i_position;
    if (u_animated) {
        ivec4 i = ivec4(i_bone_index) * 2;
		ivec4 j = i + 1;

        vec4 real0 = u_sample[i.x];
		vec4 real1 = u_sample[i.y];
		vec4 real2 = u_sample[i.z];
		vec4 real3 = u_sample[i.w];

		vec4 dual0 = u_sample[j.x];
		vec4 dual1 = u_sample[j.y];
		vec4 dual2 = u_sample[j.z];
		vec4 dual3 = u_sample[j.w];

		if (dot(real0, real1) < 0.0) {
			real1 *= -1.0;
			dual1 *= -1.0;
		}
		if (dot(real0, real2) < 0.0) {
			real2 *= -1.0;
			dual2 *= -1.0;
		}
		if (dot(real0, real3) < 0.0) {
			real3 *= -1.0;
			dual3 *= -1.0;
		}

		vec4 blend_real = real0 * i_bone_weight.x + real1 * i_bone_weight.y + real2 * i_bone_weight.z + real3 * i_bone_weight.w;
		vec4 blend_dual = dual0 * i_bone_weight.x + dual1 * i_bone_weight.y + dual2 * i_bone_weight.z + dual3 * i_bone_weight.w;
        float len = 1.0 / length(blend_real);
        blend_real *= len;
		blend_dual *= len;

		position = dq_transform(blend_real, blend_dual, position);

    }
    gl_Position = u_mvp_matrix * vec4(position, 1.0);

    v_color = i_color;
    v_uv = i_uv;
}
