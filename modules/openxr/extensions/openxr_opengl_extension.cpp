/*************************************************************************/
/*  openxr_opengl_extension.cpp                                          */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "../extensions/openxr_opengl_extension.h"
#include "../openxr_util.h"
#include "drivers/gles3/effects/copy_effects.h"
#include "drivers/gles3/storage/texture_storage.h"
#include "servers/rendering/rendering_server_globals.h"
#include "servers/rendering_server.h"

OpenXROpenGLExtension::OpenXROpenGLExtension(OpenXRAPI *p_openxr_api) :
		OpenXRGraphicsExtensionWrapper(p_openxr_api) {

#ifdef ANDROID
	request_extensions[XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME] = nullptr;
	request_extensions[XR_KHR_ANDROID_THREAD_SETTINGS_EXTENSION_NAME] = nullptr;
#else
	request_extensions[XR_KHR_OPENGL_ENABLE_EXTENSION_NAME] = nullptr;
#endif

	ERR_FAIL_NULL(openxr_api);
}

OpenXROpenGLExtension::~OpenXROpenGLExtension() {
}

void OpenXROpenGLExtension::on_instance_created(const XrInstance p_instance) {
	ERR_FAIL_NULL(openxr_api);

}

bool OpenXROpenGLExtension::check_graphics_api_support(XrVersion p_desired_version) {
	ERR_FAIL_NULL_V(openxr_api, false);

	return true;
}

void *OpenXROpenGLExtension::set_session_create_and_get_next_pointer(void *p_next_pointer) {

#ifdef WIN32
	graphics_binding_gl = XrGraphicsBindingOpenGLWin32KHR{
		.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR,
		.next = p_next_pointer,
	};

	graphics_binding_gl.hDC = (HDC)os->get_native_handle(OS::WINDOW_VIEW);
	graphics_binding_gl.hGLRC = (HGLRC)os->get_native_handle(OS::OPENGL_CONTEXT);
#elif ANDROID
	graphics_binding_gl = XrGraphicsBindingOpenGLESAndroidKHR{
		.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR,
		.next = p_next_pointer,
	};

	graphics_binding_gl.display = eglGetCurrentDisplay();
	graphics_binding_gl.config = (EGLConfig)0; // https://github.com/KhronosGroup/OpenXR-SDK-Source/blob/master/src/tests/hello_xr/graphicsplugin_opengles.cpp#L122
	graphics_binding_gl.context = eglGetCurrentContext();
#else
	graphics_binding_gl = (XrGraphicsBindingOpenGLXlibKHR){
		.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR,
		.next = p_next_pointer,
	};

	void *display_handle = (void *)os->get_native_handle(OS::DISPLAY_HANDLE);
	void *glxcontext_handle = (void *)os->get_native_handle(OS::OPENGL_CONTEXT);
	void *glxdrawable_handle = (void *)os->get_native_handle(OS::WINDOW_HANDLE);

	graphics_binding_gl.xDisplay = (Display *)display_handle;
	graphics_binding_gl.glxContext = (GLXContext)glxcontext_handle;
	graphics_binding_gl.glxDrawable = (GLXDrawable)glxdrawable_handle;

	if (graphics_binding_gl.xDisplay == nullptr) {
		Godot::print("OpenXR Failed to get xDisplay from Godot, using XOpenDisplay(nullptr)");
		graphics_binding_gl.xDisplay = XOpenDisplay(nullptr);
	}
	if (graphics_binding_gl.glxContext == nullptr) {
		Godot::print("OpenXR Failed to get glxContext from Godot, using glXGetCurrentContext()");
		graphics_binding_gl.glxContext = glXGetCurrentContext();
	}
	if (graphics_binding_gl.glxDrawable == 0) {
		Godot::print("OpenXR Failed to get glxDrawable from Godot, using glXGetCurrentDrawable()");
		graphics_binding_gl.glxDrawable = glXGetCurrentDrawable();
	}

	// spec says to use proper values but runtimes don't care
	graphics_binding_gl.visualid = 0;
	graphics_binding_gl.glxFBConfig = 0;
#endif

	return &graphics_binding_gl;
}

void OpenXROpenGLExtension::get_usable_swapchain_formats(Vector<int64_t> &p_usable_swap_chains) {
}

void OpenXROpenGLExtension::get_usable_depth_formats(Vector<int64_t> &p_usable_swap_chains) {
}

bool OpenXROpenGLExtension::get_swapchain_image_data(XrSwapchain p_swapchain, int64_t p_swapchain_format, uint32_t p_width, uint32_t p_height, uint32_t p_sample_count, uint32_t p_array_size, void **r_swapchain_graphics_data) {
	return true;
}

bool OpenXROpenGLExtension::create_projection_fov(const XrFovf p_fov, double p_z_near, double p_z_far, Projection &r_camera_matrix) {
	XrMatrix4x4f matrix;
	XrMatrix4x4f_CreateProjectionFov(&matrix, GRAPHICS_OPENGL, p_fov, (float)p_z_near, (float)p_z_far);

	for (int j = 0; j < 4; j++) {
		for (int i = 0; i < 4; i++) {
			r_camera_matrix.columns[j][i] = matrix.m[j * 4 + i];
		}
	}

	return true;
}

RID OpenXROpenGLExtension::get_texture(void *p_swapchain_graphics_data, int p_image_index) {
}

void OpenXROpenGLExtension::cleanup_swapchain_graphics_data(void **p_swapchain_graphics_data) {
}

String OpenXROpenGLExtension::get_swapchain_format_name(int64_t p_swapchain_format) const {
}
