/* clang-format off */
#[modes]

#[specializations]

USE_MULTIVIEW = false
FORMAT_LUMINANCE_ALPHA = false

#[vertex]

layout(location = 0) out vec2 uv_interp;

void main() {
	vec2 base_arr[3] = vec2[](vec2(-1.0, -1.0), vec2(-1.0, 3.0), vec2(3.0, -1.0));
	gl_Position = vec4(base_arr[gl_VertexIndex], 0.0, 1.0);
	uv_interp.xy = clamp(gl_Position.xy, vec2(0.0, 0.0), vec2(1.0, 1.0)) * 2.0; // saturate(x) * 2.0
}

#[fragment]

#ifdef USE_MULTIVIEW
uniform highp sampler2DArray env_depth_map; // texunit:-7
#else
uniform highp sampler2D env_depth_map; // texunit:-7
#endif

void main() {
	float multiplier = 1.0;

#ifdef USE_MULTIVIEW
	vec3 uv = vec3(uv_interp, ViewIndex);
#else
	vec2 uv = uv_interp;
#endif

#ifdef FORMAT_LUMINANCE_ALPHA
	vec2 packed_depth = texture(env_depth_map,  uv).ra;
	float depth = dot(packed_depth, vec2(255.0, 256.0 * 255.0)) * multiplier;
#else
	float depth = texture(env_depth_map, uv).r * multiplier;
#endif

	// @todo Actually reproject this data.
	gl_FragDepth = depth;
}
