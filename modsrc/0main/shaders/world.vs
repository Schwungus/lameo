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

uniform float u_time;
uniform vec4 u_wind;

uniform bool u_animated;
uniform vec4 u_sample[2 * MAX_BONES];

uniform vec2 u_scroll;
uniform vec3 u_material_wind;

// Simplex 4D Noise
// by Ian McEwan, Ashima Arts
vec4 permute(vec4 x) {
	return mod(((x * 34.0) + 1.0) * x, 289.0);
}

float permute(float x) {
	return floor(mod(((x * 34.0) + 1.0) * x, 289.0));
}

vec4 taylor_inv_sqrt(vec4 r) {
	return 1.79284291400159 - 0.85373472095314 * r;
}

float taylor_inv_sqrt(float r) {
	return 1.79284291400159 - 0.85373472095314 * r;
}

vec4 grad4(float j, vec4 ip) {
	const vec4 ones = vec4(1.0, 1.0, 1.0, -1.0);
	vec4 p, s;

	p.xyz = floor(fract(vec3(j) * ip.xyz) * 7.0) * ip.z - 1.0;
	p.w = 1.5 - dot(abs(p.xyz), ones.xyz);
	s = vec4(lessThan(p, vec4(0.0)));
	p.xyz = p.xyz + (s.xyz * 2.0 - 1.0) * s.www;

	return p;
}

float snoise(vec4 v) {
	const vec2 C = vec2(0.138196601125010504,  // (5 - sqrt(5))/20  G4
						0.309016994374947451); // (sqrt(5) - 1)/4   F4

	// First corner
	vec4 i  = floor(v + dot(v, C.yyyy));
	vec4 x0 = v - i + dot(i, C.xxxx);

	// Other corners
	// Rank sorting originally contributed by Bill Licea-Kane, AMD (formerly ATI)
	vec4 i0;
	vec3 is_x = step(x0.yzw, x0.xxx);
	vec3 is_yz = step(x0.zww, x0.yyz);

	i0.x = is_x.x + is_x.y + is_x.z;
	i0.yzw = 1.0 - is_x;
	i0.y += is_yz.x + is_yz.y;
	i0.zw += 1.0 - is_yz.xy;
	i0.z += is_yz.z;
	i0.w += 1.0 - is_yz.z;

	// i0 now contains the unique values 0,1,2,3 in each channel
	vec4 i3 = clamp(i0, 0.0, 1.0);
	vec4 i2 = clamp(i0 - 1.0, 0.0, 1.0);
	vec4 i1 = clamp(i0 - 2.0, 0.0, 1.0);

	vec4 x1 = x0 - i1 + 1.0 * C.xxxx;
	vec4 x2 = x0 - i2 + 2.0 * C.xxxx;
	vec4 x3 = x0 - i3 + 3.0 * C.xxxx;
	vec4 x4 = x0 - 1. + 4.0 * C.xxxx;

	// Permutations
	i = mod(i, 289.0);

	float j0 = permute(permute(permute(permute(i.w) + i.z) + i.y) + i.x);
	vec4 j1 = permute(permute(permute(permute(
			  i.w + vec4(i1.w, i2.w, i3.w, 1.0))
			+ i.z + vec4(i1.z, i2.z, i3.z, 1.0))
			+ i.y + vec4(i1.y, i2.y, i3.y, 1.0))
			+ i.x + vec4(i1.x, i2.x, i3.x, 1.0));

	// Gradients
	// ( 7*7*6 points uniformly over a cube, mapped onto a 4-octahedron.)
	// 7*7*6 = 294, which is close to the ring size 17*17 = 289.
	vec4 ip = vec4(1.0 / 294.0, 1.0 / 49.0, 1.0 / 7.0, 0.0);
	vec4 p0 = grad4(j0, ip);
	vec4 p1 = grad4(j1.x, ip);
	vec4 p2 = grad4(j1.y, ip);
	vec4 p3 = grad4(j1.z, ip);
	vec4 p4 = grad4(j1.w, ip);

	// Normalise gradients
	vec4 norm = taylor_inv_sqrt(vec4(dot(p0, p0), dot(p1, p1), dot(p2, p2), dot(p3, p3)));

	p0 *= norm.x;
	p1 *= norm.y;
	p2 *= norm.z;
	p3 *= norm.w;
	p4 *= taylor_inv_sqrt(dot(p4,p4));

	// Mix contributions from the five corners
	vec3 m0 = max(0.6 - vec3(dot(x0, x0), dot(x1, x1), dot(x2, x2)), 0.0);
	vec2 m1 = max(0.6 - vec2(dot(x3, x3), dot(x4, x4)), 0.0);

	m0 = m0 * m0;
	m1 = m1 * m1;

	return 49.0 * (dot(m0 * m0, vec3(dot(p0, x0), dot(p1, x1), dot(p2, x2)))
			    + dot(m1 * m1, vec2(dot(p3, x3), dot(p4, x4))));
}

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

    if (u_material_wind.x > 0.0) {
		float wind_time = u_time * u_material_wind.y;
		float wind_weight = (1.0 - (u_material_wind.z * clamp(i_uv.y, 0.0, 1.0))) * u_wind.w * u_material_wind.x;

        vec3 v = i_position;
		position.x += u_wind.x * snoise(vec4( v.x, -v.y, -v.z, wind_time)) * wind_weight * min(length(u_model_matrix[0]), 1.0);
		position.y += u_wind.y * snoise(vec4(-v.x,  v.y, -v.z, wind_time)) * wind_weight * min(length(u_model_matrix[1]), 1.0);
		position.z += u_wind.z * snoise(vec4(-v.x, -v.y,  v.z, wind_time)) * wind_weight * min(length(u_model_matrix[2]), 1.0);
	}

    gl_Position = u_mvp_matrix * vec4(position, 1.0);

    v_color = i_color;
    v_uv = i_uv;
    v_uv.xy += u_time * u_scroll;
}
