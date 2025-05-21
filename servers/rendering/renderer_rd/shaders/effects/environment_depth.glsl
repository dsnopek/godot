#[vertex]

#version 450

#VERSION_DEFINES

#ifdef USE_MULTIVIEW
#ifdef has_VK_KHR_multiview
#extension GL_EXT_multiview : enable
#define ViewIndex gl_ViewIndex
#else // has_VK_KHR_multiview
#define ViewIndex 0
#endif // has_VK_KHR_multiview
#endif // USE_MULTIVIEW

layout(location = 0) out vec2 uv_interp;

void main() {
	vec2 base_arr[3] = vec2[](vec2(-1.0, -1.0), vec2(-1.0, 3.0), vec2(3.0, -1.0));
	gl_Position = vec4(base_arr[gl_VertexIndex], 0.0, 1.0);
	uv_interp.xy = clamp(gl_Position.xy, vec2(0.0, 0.0), vec2(1.0, 1.0)) * 2.0; // saturate(x) * 2.0
}

#[fragment]

#version 450

#VERSION_DEFINES

#ifdef USE_MULTIVIEW
#ifdef has_VK_KHR_multiview
#extension GL_EXT_multiview : enable
#define ViewIndex gl_ViewIndex
#else // has_VK_KHR_multiview
#define ViewIndex 0
#endif // has_VK_KHR_multiview
#endif // USE_MULTIVIEW

#ifdef USE_MULTIVIEW
layout(set = 0, binding = 0) uniform sampler2DArray env_depth_map;
#else
layout(set = 0, binding = 0) uniform sampler2D env_depth_map;
#endif

void main() {
	float multiplier = 1.0;

#ifdef USE_MULTIVIEW
	vec3 uv = vec3(uv_interp, ViewIndex);
#else
	vec2 uv = uv_interp;
#endif

#ifdef FORMAT_LUMINANCE_ALPHA
	vec2 packed_depth = texture(env_depth_map, uv).ra;
	float depth = dot(packed_depth, vec2(255.0, 256.0 * 255.0)) * multiplier;
#else
	float depth = texture(env_depth_map, uv).r * multiplier;
#endif

	// @todo Actually reproject this data.
	gl_FragDepth = depth;
}
