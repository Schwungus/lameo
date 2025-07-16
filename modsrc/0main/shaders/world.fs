#version 330 core

out vec4 o_color;

in vec4 v_color;
in vec4 v_uv;

uniform sampler2D u_texture;
uniform bool u_has_blend_texture;
uniform sampler2D u_blend_texture;

uniform float u_alpha_test;
uniform vec4 u_stencil;

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

    o_color = v_color * sample;
    if (o_color.a <= (bayer8(gl_FragCoord.xy) + 0.003921568627451))
		discard;
	o_color.a = 1.0;
    o_color.rgb = mix(o_color.rgb, u_stencil.rgb, u_stencil.a);
}
