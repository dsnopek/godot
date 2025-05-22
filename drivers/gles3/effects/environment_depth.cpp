/**************************************************************************/
/*  environment_depth.cpp                                                 */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#ifdef GLES3_ENABLED

#include "environment_depth.h"

#include "../storage/texture_storage.h"

using namespace GLES3;

EnvironmentDepth *EnvironmentDepth::singleton = nullptr;

EnvironmentDepth *EnvironmentDepth::get_singleton() {
	return singleton;
}

EnvironmentDepth::EnvironmentDepth() {
	singleton = this;

	env_depth.shader.initialize();
	env_depth.shader_version = env_depth.shader.version_create();

	{
		glGenBuffers(1, &screen_triangle);
		glBindBuffer(GL_ARRAY_BUFFER, screen_triangle);

		const float qv[6] = {
			-1.0f,
			-1.0f,
			3.0f,
			-1.0f,
			-1.0f,
			3.0f,
		};

		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6, qv, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glGenVertexArrays(1, &screen_triangle_array);
		glBindVertexArray(screen_triangle_array);
		glBindBuffer(GL_ARRAY_BUFFER, screen_triangle);
		glVertexAttribPointer(RS::ARRAY_VERTEX, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
		glEnableVertexAttribArray(RS::ARRAY_VERTEX);
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}

EnvironmentDepth::~EnvironmentDepth() {
	glDeleteBuffers(1, &screen_triangle);
	glDeleteVertexArrays(1, &screen_triangle_array);

	env_depth.shader.version_free(env_depth.shader_version);

	singleton = nullptr;
}

void EnvironmentDepth::fill_depth_buffer(RID p_depth_map, bool p_use_multiview) {
	uint64_t specialization = p_use_multiview ? EnvironmentDepthShaderGLES3::USE_MULTIVIEW : 0;

	bool success = env_depth.shader.version_bind_shader(env_depth.shader_version, EnvironmentDepthShaderGLES3::MODE_DEFAULT, specialization);
	if (!success) {
		return;
	}

	TextureStorage::get_singleton()->texture_bind(p_depth_map, 0);

	// Sampling a depth buffer won't work unless the min and mag filter are nearest.
	GLenum target = p_use_multiview ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindVertexArray(screen_triangle_array);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(0);
}

#endif // GLES3_ENABLED
