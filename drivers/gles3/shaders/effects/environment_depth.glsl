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

// @todo These uniforms should probably be in a UBO, but I was lazy - do this before taking out of draft!

uniform highp mat4 depth_proj_left;
uniform highp mat4 depth_proj_right;

uniform highp mat4 depth_inv_proj_left;
uniform highp mat4 depth_inv_proj_right;

uniform highp mat4 cur_proj_left;
uniform highp mat4 cur_proj_right;

uniform highp mat4 cur_inv_proj_left;
uniform highp mat4 cur_inv_proj_right;

in vec2 uv_interp;

void main() {
	float multiplier = 1.0;

	mat4 depth_proj = ViewIndex == uint(0) ? depth_proj_left : depth_proj_right;
	mat4 cur_proj = ViewIndex == uint(0) ? cur_proj_left : cur_proj_right;
	mat4 depth_inv_proj = ViewIndex == uint(0) ? depth_inv_proj_left : depth_inv_proj_right;
	mat4 cur_inv_proj = ViewIndex == uint(0) ? cur_inv_proj_left : cur_inv_proj_right;

	vec4 clip = vec4(uv_interp * 2.0 - 1.0, 1.0, 1.0);
	vec4 world_pos = cur_inv_proj * clip;
	world_pos /= world_pos.w;

	vec4 reprojected = depth_proj * world_pos;
	reprojected /= reprojected.w;
	vec2 reprojected_uv = reprojected.xy * 0.5 + 0.5;

#ifdef USE_MULTIVIEW
	vec3 depth_coord = vec3(reprojected_uv, ViewIndex);
#else
	vec2 depth_coord = reprojected_uv;
#endif

#ifdef FORMAT_LUMINANCE_ALPHA
	vec2 packed_depth = texture(env_depth_map, depth_coord).ra;
	float depth = dot(packed_depth, vec2(255.0, 256.0 * 255.0));
#else
	float depth = texture(env_depth_map, depth_coord).r;
#endif

	if (depth == 0.0) {
		discard;
	}

	// For debugging:
	//gl_FragDepth = 1.0 - depth;

	// Now that we've got the environment depth from the depth map, we need to reproject AGAIN,
	// so that the depth value will be scaled per Godot's projection matrix.

	vec4 clip_back = vec4(reprojected.xy, depth * 2.0 - 1.0, 1.0);
	vec4 world_back = depth_inv_proj * clip_back;
	world_back /= world_back.w;

	vec4 cur_clip = cur_proj * world_back;
	float ndc_z = cur_clip.z / cur_clip.w;

	gl_FragDepth = 1.0 - (ndc_z * 0.5 + 0.5);
}
