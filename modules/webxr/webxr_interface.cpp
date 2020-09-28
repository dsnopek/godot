/*************************************************************************/
/*  webxr_interface.cpp                                                  */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2020 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2020 Godot Engine contributors (cf. AUTHORS.md).   */
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

#ifdef JAVASCRIPT_ENABLED

#include <stdlib.h>
#include "webxr_interface.h"
#include "core/os/input.h"
#include "core/os/os.h"
#include "servers/visual/visual_server_globals.h"
#include "emscripten.h"

StringName WebXRInterface::get_name() const {
	return "WebXR";
};

int WebXRInterface::get_capabilities() const {
	return ARVRInterface::ARVR_STEREO;
};

void WebXRInterface::_bind_methods() {
}

bool WebXRInterface::is_stereo() {
	// @todo WebXR can be mono! So, how do we know? Count the views in the frame?
	return true;
};

bool WebXRInterface::is_initialized() const {
	return (initialized);
};

bool WebXRInterface::initialize() {
	ARVRServer *arvr_server = ARVRServer::get_singleton();
	ERR_FAIL_NULL_V(arvr_server, false);

	//printf("My own little debugging\n");

	if (!initialized) {
		/* clang-format off */
		bool vr_supported = EM_ASM_INT({
			return !!navigator.xr;
		});
		/* clang-format on */

		if (!vr_supported) {
			return false;
		}

		// make this our primary interface
		arvr_server->set_primary_interface(this);

		initialized = true;

		EM_ASM({
			console.log(Object.keys(Module));
			console.log(Object.keys(Module['InternalBrowser']));
			navigator.xr.isSessionSupported('immersive-vr').then(function () {
				console.log('immersive-vr is supported');
				navigator.xr.requestSession('immersive-vr').then(function (session) {
					console.log('got session');
					Module['webxr_session'] = session;
					let gl = Module['ctx'];
					gl.makeXRCompatible().then(function () {
						console.log('made compatible');
						session.updateRenderState({
							baseLayer: new XRWebGLLayer(session, gl)
						});
						session.requestReferenceSpace('local').then(function (refSpace) {
							console.log('requested space');
							Module['webxr_space'] = refSpace;

							// Monkey patch window.requestAnimationFrame so we can replace it with the WebXR equivalent.
							// This is called via emscripten_set_main_loop().
							Module['webxr_frame'] = null;
							if (!Module['webxr_orig_requestAnimationFrame']) {
								Module['webxr_orig_requestAnimationFrame'] = Module.InternalBrowser.requestAnimationFrame;
								Module.InternalBrowser.requestAnimationFrame = function (callback) {
									//console.log('request frame');
									let onFrame = function (time, frame) {
										//console.log('got a frame');
										Module['webxr_frame'] = frame;
										Module['webxr_pose'] = frame.getViewerPose(Module['webxr_space']);
										callback(time);
									};
									session.requestAnimationFrame(onFrame);
								};
							}
						});
					});
				});
			});
		});
	};

	return true;
};

void WebXRInterface::uninitialize() {
	if (initialized) {
		ARVRServer *arvr_server = ARVRServer::get_singleton();
		if (arvr_server != NULL) {
			// no longer our primary interface
			arvr_server->clear_primary_interface_if(this);
		}

		// Undo monkey-patch of window.requestAnimationFrame() to restore the non-XR main loop.
		EM_ASM({
			if (Module['webxr_orig_requestAnimationFrame']) {
				Module.InternalBrowser.requestAnimationFrame = Module['webxr_orig_requestAnimationFrame'];
				delete Module['webxr_orig_requestAnimationFrame'];
			}
		});

		initialized = false;
	};
};

bool WebXRInterface::_have_frame() {
	return (bool) EM_ASM_INT({
		return !!Module['webxr_session'] && !!Module['webxr_pose'];
	});
}

Size2 WebXRInterface::get_render_targetsize() {
	// @todo This method is untested!

	Size2 target_size;

	//EM_ASM({ console.log("get_render_targetsize()"); });

	if (!initialized || !_have_frame()) {
		// As a default, use half the window size.
		target_size = OS::get_singleton()->get_window_size();
		target_size.width /= 2.0;
		return target_size;
	}

	int32_t* js_size = (int32_t*) EM_ASM_INT({
		let glLayer = Module['webxr_frame'].session.renderState.baseLayer;
		let view = Module['webxr_pose'].views[0];
		let viewport = glLayer.getViewport(view);

		//console.log("targetsize javascript viewport:");
		//console.log(viewport.width + " " + viewport.height);

		let buf = Module._malloc(2 * 4);
		setValue(buf + 0, viewport.width, 'i32');
		setValue(buf + 4, viewport.height, 'i32');
		return buf;
	});

	target_size.width = js_size[0];
	target_size.height = js_size[1];

	free(js_size);

	EM_ASM({
		//console.log("targetsize C++ viewport:");
		//console.log($0 + " " + $1);
	}, target_size.width, target_size.height);

	return target_size;
};

Transform WebXRInterface::get_transform_for_eye(ARVRInterface::Eyes p_eye, const Transform &p_cam_transform) {
	Transform transform_for_eye;

	// @todo In 3DOF where we only have rotation, we should take the position from the p_cam_transform, I think?

	if (!initialized || !_have_frame()) {
		transform_for_eye = p_cam_transform;
		return transform_for_eye;
	}

	int view_index = (p_eye == ARVRInterface::EYE_RIGHT) ? 1 : 0;

	float* js_matrix = (float*) EM_ASM_INT({
		const matrix = Module['webxr_pose'].views[$0].transform.inverse.matrix;
		let buf = Module._malloc(16 * 4);
		for (let i = 0; i < 16; i++) {
			setValue(buf + (i * 4), matrix[i], 'float')
		}
		return buf;
	}, view_index);

	transform_for_eye.basis.elements[0].x = js_matrix[0];
	transform_for_eye.basis.elements[0].y = js_matrix[1];
	transform_for_eye.basis.elements[0].z = js_matrix[2];
	transform_for_eye.basis.elements[1].x = js_matrix[4];
	transform_for_eye.basis.elements[1].y = js_matrix[5];
	transform_for_eye.basis.elements[1].z = js_matrix[6];
	transform_for_eye.basis.elements[2].x = js_matrix[8];
	transform_for_eye.basis.elements[2].y = js_matrix[9];
	transform_for_eye.basis.elements[2].z = js_matrix[10];
	transform_for_eye.origin.x = -js_matrix[12];
	transform_for_eye.origin.y = -js_matrix[13];
	transform_for_eye.origin.z = -js_matrix[14];

	free(js_matrix);

	return transform_for_eye;
};

CameraMatrix WebXRInterface::get_projection_for_eye(ARVRInterface::Eyes p_eye, real_t p_aspect, real_t p_z_near, real_t p_z_far) {
	CameraMatrix eye;

	if (!initialized || !_have_frame()) {
		return eye;
	}

	int view_index = (p_eye == ARVRInterface::EYE_RIGHT) ? 1 : 0;

	float* js_matrix = (float*) EM_ASM_INT({
		const matrix = Module['webxr_pose'].views[$0].projectionMatrix;
		let buf = Module._malloc(16 * 4);
		for (let i = 0; i < 16; i++) {
			setValue(buf + (i * 4), matrix[i], 'float')
		}
		return buf;
	}, view_index);

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			eye.matrix[i][j] = js_matrix[(i * 4) + j];
		}
	}

	free(js_matrix);

	// Copied from MobileVRInterface::get_projection_for_eye().
	//eye.set_perspective(60.0, p_aspect, p_z_near, p_z_far, false);

	return eye;
}

void WebXRInterface::commit_for_eye(ARVRInterface::Eyes p_eye, RID p_render_target, const Rect2 &p_screen_rect) {
	if (!initialized || !_have_frame()) {
		return;
	}

	// Change current render target to the main screen.
	//VSG::rasterizer->set_current_render_target(RID());

	int view_index = (p_eye == ARVRInterface::EYE_RIGHT) ? 1 : 0;

	int32_t* js_viewport = (int32_t*) EM_ASM_INT({
		let glLayer = Module['webxr_frame'].session.renderState.baseLayer;
		let view = Module['webxr_pose'].views[$0];
		let viewport = glLayer.getViewport(view);
		let gl = Module['ctx'];

		// @todo We really need to re-enable this!
		//gl.bindFramebuffer(gl.FRAMEBUFFER, glLayer.framebuffer);
		//gl.viewport(viewport.x, viewport.y, viewport.width, viewport.height);

		//console.log("commit javascript viewport: " + viewport.x + " " + viewport.y + " " + viewport.width + " " + viewport.height);

		let buf = Module._malloc(4 * 4);
		setValue(buf + 0, viewport.x, 'i32');
		setValue(buf + 4, viewport.y, 'i32');
		setValue(buf + 8, viewport.width, 'i32');
		setValue(buf + 12, viewport.height, 'i32');
		return buf;
	}, view_index);

	//printf("js_viewport: %d\n", (int)js_viewport);

	Rect2 viewport;
	viewport.position.x = js_viewport[0];
	viewport.position.y = js_viewport[1];
	viewport.size.width = js_viewport[2];
	viewport.size.height = js_viewport[3];

	free(js_viewport);

	//printf("commit c++ viewport: %f %f %f %f\n", viewport.position.x, viewport.position.y, viewport.size.width, viewport.size.height);

	// Temporary: just get something on the screen!
	VSG::rasterizer->set_current_render_target(RID());
	VSG::rasterizer->blit_render_target_to_screen(p_render_target, viewport, 0);

	//VSG::rasterizer->blit_render_target_to_screen(p_render_target, viewport, 0);
	//VSG::rasterizer->blit_render_target_to_current_framebuffer(p_render_target, viewport);
};

void WebXRInterface::process() {
	if (initialized) {
		//set_position_from_sensors();
	};
};

void WebXRInterface::notification(int p_what){
	// nothing to do here, I guess we could pauze our sensors...
}

WebXRInterface::WebXRInterface() {
	initialized = false;
};

WebXRInterface::~WebXRInterface() {
	// and make sure we cleanup if we haven't already
	if (is_initialized()) {
		uninitialize();
	};
};

#endif // JAVASCRIPT_ENABLED

