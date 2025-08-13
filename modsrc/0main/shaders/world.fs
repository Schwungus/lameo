#version 330 core

#define MAX_ROOM_LIGHTS 8

#define RL_OFF 0
#define RL_NO_LIGHTMAP 1
#define RL_ON 2

#define RL_SUN 0
#define RL_POINT 1
#define RL_SPOT 2

#define RL_ACTIVE 0
#define RL_TYPE 1
#define RL_X 2
#define RL_Y 3
#define RL_Z 4
#define RL_R 5
#define RL_G 6
#define RL_B 7
#define RL_A 8

#define RL_SUN_NX 9
#define RL_SUN_NY 10
#define RL_SUN_NZ 11

#define RL_POINT_NEAR 9
#define RL_POINT_FAR 10

#define RL_SPOT_NX 9
#define RL_SPOT_NY 10
#define RL_SPOT_NZ 11
#define RL_SPOT_RANGE 12
#define RL_SPOT_IN 13
#define RL_SPOT_OUT 14

#define RL_SIZE 15
#define RL_ARRAY_SIZE (MAX_ROOM_LIGHTS * RL_SIZE)

out vec4 o_color;

in vec3 v_position;
in vec3 v_world_position;
in vec3 v_view_position;
in vec3 v_normal;
in vec4 v_color;
in vec4 v_uv;
in float v_rimlight;

uniform sampler2D u_texture;
uniform bool u_has_blend_texture;
uniform sampler2D u_blend_texture;

uniform float u_alpha_test;
uniform vec4 u_color;
uniform vec4 u_stencil;

uniform vec4 u_ambient;
uniform float u_lights[RL_ARRAY_SIZE];
uniform vec2 u_fog_distance;
uniform vec4 u_fog_color;
uniform float u_bright;
uniform bool u_half_lambert;
uniform float u_cel;
uniform vec4 u_specular;

uniform bool u_has_lightmap;
uniform sampler2D u_lightmap;

float matdot(float dotp) {
	dotp = u_half_lambert ? pow((dotp * 0.5) + 0.5, 2.0) : max(dotp, 0.0);
	return smoothstep(0.0 + u_cel, 1.0 - u_cel, dotp);
}

float matdot(vec3 a, vec3 b) {
	float dotp = u_half_lambert ? pow((dot(a, b) * 0.5) + 0.5, 2.0) : max(dot(a, b), 0.0);
	return smoothstep(0.0 + u_cel, 1.0 - u_cel, dotp);
}

// FabriceNeyret2, ollj, Tech_
float bayer2(vec2 a) {
	a = floor(a);
	return fract(dot(a, vec2(0.5, a.y * 0.75)));
}

#define bayer4(a) (bayer2(0.5 * a) * 0.25 + bayer2(a))
#define bayer8(a) (bayer4(0.5 * a) * 0.25 + bayer2(a))

void main() {
    vec4 sample = texture(u_texture, v_uv.xy);
    if (u_has_blend_texture)
        sample = mix(texture(u_blend_texture, v_uv.xy), sample, v_color.a);
    if (u_alpha_test > 0.0) {
        if (sample.a < u_alpha_test)
            discard;
        sample.a = 1.0;
    }

    vec3 reflection = normalize(reflect(v_view_position, v_normal));
    vec4 lighting = u_has_lightmap ? (texture(u_lightmap, v_uv.zw) * 2.0) : u_ambient;
    float specular = 0.0;
    for (int i = 0; i < RL_ARRAY_SIZE; i += RL_SIZE) {
        int light_active = int(u_lights[i + RL_ACTIVE]);
        if (light_active <= RL_OFF || (light_active == RL_NO_LIGHTMAP && u_has_lightmap))
            continue;

        int light_type = int(u_lights[i + RL_TYPE]);
        if (light_type == RL_SUN) {
            vec4 light_color = vec4(u_lights[i + RL_R], u_lights[i + RL_G], u_lights[i + RL_B], u_lights[i + RL_A]);
            vec3 light_normal = -normalize(vec3(u_lights[i + RL_SUN_NX], u_lights[i + RL_SUN_NY], u_lights[i + RL_SUN_NZ]));

            lighting += matdot(v_normal, light_normal) * light_color;
            specular += matdot(reflection, light_normal);
        } else if (light_type == RL_POINT) {
            vec3 light_pos = vec3(u_lights[i + RL_X], u_lights[i + RL_Y], u_lights[i + RL_Z]);
            vec4 light_color = vec4(u_lights[i + RL_R], u_lights[i + RL_G], u_lights[i + RL_B], u_lights[i + RL_A]);
            vec2 light_range = vec2(u_lights[i + RL_POINT_NEAR], u_lights[i + RL_POINT_FAR]);

            vec3 dir = normalize(v_world_position - light_pos);
            float att = max((light_range.y - distance(v_world_position, light_pos)) / (light_range.y - light_range.x), 0.0);
            lighting += att * light_color * matdot(v_normal, -dir);
            specular += att * matdot(reflection, dir);
        } else if (light_type == RL_SPOT) {
            vec3 light_pos = vec3(u_lights[i + RL_X], u_lights[i + RL_Y], u_lights[i + RL_Z]);
            vec4 light_color = vec4(u_lights[i + RL_R], u_lights[i + RL_G], u_lights[i + RL_B], u_lights[i + RL_A]);
            vec3 light_normal = -normalize(vec3(u_lights[i + RL_SPOT_NX], u_lights[i + RL_SPOT_NY], u_lights[i + RL_SPOT_NZ]));
            float light_range = u_lights[i + RL_SPOT_RANGE];
            vec2 light_cutoff = vec2(u_lights[i + RL_SPOT_IN], u_lights[i + RL_SPOT_OUT]);

            vec3 dir = v_world_position - light_pos;
            float dist = length(dir);
            dir = normalize(-dir);
            float diff = max(dot(dir, light_normal), 0.0);
            float att = clamp((diff - light_cutoff.y) / (light_cutoff.x - light_cutoff.y), 0.0, 1.0) * max((light_range - dist) / light_range, 0.0);
            lighting += att * light_color * matdot(v_normal, dir);
            specular += att * matdot(reflection, dir);
        }
    }

    o_color = v_color * sample * mix(lighting, vec4(1.0), u_bright);
    o_color.rgb += (pow(mix(specular, 0.0, u_bright), u_specular.y) * u_specular.x) + (pow(mix(1.0 - matdot(v_rimlight), 0.0, u_bright), u_specular.w) * u_specular.z);

    float fog = clamp((length(v_position) - u_fog_distance.x) / (u_fog_distance.y - u_fog_distance.x), 0.0, 1.0);
    o_color.rgb = mix(o_color.rgb, u_fog_color.rgb, fog);
    o_color.a *= mix(1.0, u_fog_color.a, fog);

    if (o_color.a <= (bayer8(gl_FragCoord.xy) + 0.003921568627451))
		discard;
    o_color.a = 1.0;
    o_color.rgb = mix(o_color.rgb, u_stencil.rgb, u_stencil.a);
}
