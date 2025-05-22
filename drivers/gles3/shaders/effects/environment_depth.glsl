/* clang-format off */
#[modes]

mode_default =

#[specializations]

USE_MULTIVIEW = false
FORMAT_LUMINANCE_ALPHA = false

#[vertex]

layout(location = 0) in vec2 vertex_attrib;

out vec2 uv_interp;

void main() {
	uv_interp = vertex_attrib * 0.5 + 0.5;
	gl_Position = vec4(vertex_attrib, 1.0, 1.0);
}

#[fragment]

#ifdef USE_MULTIVIEW
uniform highp sampler2DArray env_depth_map; // texunit:0
#else
uniform highp sampler2D env_depth_map; // texunit:0
#endif

in vec2 uv_interp;

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
