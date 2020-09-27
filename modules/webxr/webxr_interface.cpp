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
	// needs stereo...
	return true;
};

bool WebXRInterface::is_initialized() const {
	return (initialized);
};

extern "C" EMSCRIPTEN_KEEPALIVE void _webxr_request_frame() {
	EM_ASM({
		let session = Module['webxr_session'];
		if (!session) {
			console.log('no session - cannot request frame');
			return;
		}

		console.log('request frame');

		Module['webxr_frame'] = null;
		let onFrame = function (time, frame) {
			Module['webxr_frame'] = frame;
		};
		session.requestAnimationFrame(onFrame);
	});
}

bool WebXRInterface::initialize() {
	ARVRServer *arvr_server = ARVRServer::get_singleton();
	ERR_FAIL_NULL_V(arvr_server, false);

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
			navigator.xr.isSessionSupported('immersive-vr').then(function () {
				console.log('immersive-vr is supported');
				navigator.xr.requestSession('immersive-vr').then(function (session) {
					console.log('got session');
					Module['webxr_session'] = session;
					// @todo Find a way to get access to the existing context rather than getting a new one
					// I know this is stored in GL.contexts[id] from Emscripten's LibraryGL, but I don't know how to actually access that...
					let ctx = engine.canvas.getContext('webgl');
					ctx.makeXRCompatible().then(function () {
						console.log('made compatible');
						session.updateRenderState({
							baseLayer: new XRWebGLLayer(session, ctx)
						});
						session.requestReferenceSpace('local').then(function (refSpace) {
							console.log('requested space');
							Module['webxr_space'] = refSpace;
							ccall("_webxr_request_frame", "void", [], []);
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

		initialized = false;
	};
};

Size2 WebXRInterface::get_render_targetsize() {
	_THREAD_SAFE_METHOD_

	Size2 target_size = OS::get_singleton()->get_window_size();
	return target_size;
};

Transform WebXRInterface::get_transform_for_eye(ARVRInterface::Eyes p_eye, const Transform &p_cam_transform) {
	_THREAD_SAFE_METHOD_

	Transform transform_for_eye;

	return transform_for_eye;
};

CameraMatrix WebXRInterface::get_projection_for_eye(ARVRInterface::Eyes p_eye, real_t p_aspect, real_t p_z_near, real_t p_z_far) {
	_THREAD_SAFE_METHOD_

	CameraMatrix eye;

	return eye;
};

void WebXRInterface::commit_for_eye(ARVRInterface::Eyes p_eye, RID p_render_target, const Rect2 &p_screen_rect) {
	_THREAD_SAFE_METHOD_

};

void WebXRInterface::process() {
	_THREAD_SAFE_METHOD_

	if (initialized) {
		//set_position_from_sensors();
	};
};

void WebXRInterface::notification(int p_what){
	_THREAD_SAFE_METHOD_

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

