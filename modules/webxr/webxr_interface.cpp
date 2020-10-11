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
#include "emscripten.h"
#include "godot_webxr.h"
#include "servers/visual/visual_server_globals.h"
#include <stdlib.h>

extern "C" EMSCRIPTEN_KEEPALIVE void _emwebxr_on_session_supported(char *p_session_mode, bool supported) {
	ARVRServer *arvr_server = ARVRServer::get_singleton();
	ERR_FAIL_NULL(arvr_server);

	Ref<ARVRInterface> interface = arvr_server->find_interface("WebXR");
	ERR_FAIL_COND(interface.is_null());

	String session_mode = String(p_session_mode);
	interface->emit_signal("session_supported", session_mode, supported);
}

extern "C" EMSCRIPTEN_KEEPALIVE void _emwebxr_on_session_started(char *p_reference_space_type) {
	ARVRServer *arvr_server = ARVRServer::get_singleton();
	ERR_FAIL_NULL(arvr_server);

	Ref<ARVRInterface> interface = arvr_server->find_interface("WebXR");
	ERR_FAIL_COND(interface.is_null());

	String reference_space_type = String(p_reference_space_type);
	((WebXRInterface *)interface.ptr())->_set_reference_space_type(reference_space_type);
	interface->emit_signal("session_started");
}

extern "C" EMSCRIPTEN_KEEPALIVE void _emwebxr_on_session_ended() {
	ARVRServer *arvr_server = ARVRServer::get_singleton();
	ERR_FAIL_NULL(arvr_server);

	Ref<ARVRInterface> interface = arvr_server->find_interface("WebXR");
	ERR_FAIL_COND(interface.is_null());

	interface->uninitialize();
	interface->emit_signal("session_ended");
}

extern "C" EMSCRIPTEN_KEEPALIVE void _emwebxr_on_session_failed(char *p_message) {
	ARVRServer *arvr_server = ARVRServer::get_singleton();
	ERR_FAIL_NULL(arvr_server);

	Ref<ARVRInterface> interface = arvr_server->find_interface("WebXR");
	ERR_FAIL_COND(interface.is_null());

	String message = String(p_message);
	interface->emit_signal("session_failed", message);
}

void WebXRInterface::is_session_supported(const String &p_session_mode) {
	godot_webxr_is_session_supported(p_session_mode.utf8().get_data());
}

void WebXRInterface::set_session_mode(String p_session_mode) {
	session_mode = p_session_mode;
}

String WebXRInterface::get_session_mode() const {
	return session_mode;
}

void WebXRInterface::set_required_features(String p_required_features) {
	required_features = p_required_features;
}

String WebXRInterface::get_required_features() const {
	return required_features;
}

void WebXRInterface::set_optional_features(String p_optional_features) {
	optional_features = p_optional_features;
}

String WebXRInterface::get_optional_features() const {
	return optional_features;
}

void WebXRInterface::set_requested_reference_space_types(String p_requested_reference_space_types) {
	requested_reference_space_types = p_requested_reference_space_types;
}

String WebXRInterface::get_requested_reference_space_types() const {
	return requested_reference_space_types;
}

void WebXRInterface::_set_reference_space_type(String p_reference_space_type) {
	reference_space_type = p_reference_space_type;
}

String WebXRInterface::get_reference_space_type() const {
	return reference_space_type;
}

void WebXRInterface::_bind_methods() {
	ClassDB::bind_method(D_METHOD("is_session_supported", "session_mode"), &WebXRInterface::is_session_supported);
	ClassDB::bind_method(D_METHOD("set_session_mode"), &WebXRInterface::set_session_mode);
	ClassDB::bind_method(D_METHOD("get_session_mode"), &WebXRInterface::get_session_mode);
	ClassDB::bind_method(D_METHOD("set_required_features"), &WebXRInterface::set_required_features);
	ClassDB::bind_method(D_METHOD("get_required_features"), &WebXRInterface::get_required_features);
	ClassDB::bind_method(D_METHOD("set_optional_features"), &WebXRInterface::set_optional_features);
	ClassDB::bind_method(D_METHOD("get_optional_features"), &WebXRInterface::get_optional_features);
	ClassDB::bind_method(D_METHOD("get_reference_space_type"), &WebXRInterface::get_reference_space_type);
	ClassDB::bind_method(D_METHOD("set_requested_reference_space_types"), &WebXRInterface::set_requested_reference_space_types);
	ClassDB::bind_method(D_METHOD("get_requested_reference_space_types"), &WebXRInterface::get_requested_reference_space_types);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "session_mode", PROPERTY_HINT_NONE), "set_session_mode", "get_session_mode");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "required_features", PROPERTY_HINT_NONE), "set_required_features", "get_required_features");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "optional_features", PROPERTY_HINT_NONE), "set_optional_features", "get_optional_features");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "requested_reference_space_types", PROPERTY_HINT_NONE), "set_requested_reference_space_types", "get_requested_reference_space_types");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "reference_space_type", PROPERTY_HINT_NONE), "", "get_reference_space_type");

	ADD_SIGNAL(MethodInfo("session_supported", PropertyInfo(Variant::STRING, "session_mode"), PropertyInfo(Variant::BOOL, "supported")));
	ADD_SIGNAL(MethodInfo("session_started"));
	ADD_SIGNAL(MethodInfo("session_ended"));
	ADD_SIGNAL(MethodInfo("session_failed", PropertyInfo(Variant::STRING, "message")));
}

StringName WebXRInterface::get_name() const {
	return "WebXR";
};

int WebXRInterface::get_capabilities() const {
	return ARVRInterface::ARVR_STEREO;
};

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

	if (!initialized) {
		if (!godot_webxr_is_supported()) {
			return false;
		}

		if (requested_reference_space_types.size() == 0) {
			return false;
		}

		// make this our primary interface
		arvr_server->set_primary_interface(this);

		initialized = true;

		godot_webxr_initialize(
				session_mode.utf8().get_data(),
				required_features.utf8().get_data(),
				optional_features.utf8().get_data(),
				requested_reference_space_types.utf8().get_data());
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

		godot_webxr_uninitialize();

		reference_space_type = "";
		initialized = false;
	};
};

Transform WebXRInterface::_js_matrix_to_transform(float *p_js_matrix) {
	Transform transform;

	transform.basis.elements[0].x = p_js_matrix[0];
	transform.basis.elements[1].x = p_js_matrix[1];
	transform.basis.elements[2].x = p_js_matrix[2];
	transform.basis.elements[0].y = p_js_matrix[4];
	transform.basis.elements[1].y = p_js_matrix[5];
	transform.basis.elements[2].y = p_js_matrix[6];
	transform.basis.elements[0].z = p_js_matrix[8];
	transform.basis.elements[1].z = p_js_matrix[9];
	transform.basis.elements[2].z = p_js_matrix[10];
	transform.origin.x = p_js_matrix[12];
	transform.origin.y = p_js_matrix[13];
	transform.origin.z = p_js_matrix[14];

	return transform;
}

Size2 WebXRInterface::get_render_targetsize() {
	Size2 target_size;

	int *js_size = godot_webxr_get_render_targetsize();
	if (!initialized || js_size == nullptr) {
		// As a default, use half the window size.
		target_size = OS::get_singleton()->get_window_size();
		target_size.width /= 2.0;
		return target_size;
	}

	target_size.width = js_size[0];
	target_size.height = js_size[1];

	free(js_size);

	return target_size;
};

Transform WebXRInterface::get_transform_for_eye(ARVRInterface::Eyes p_eye, const Transform &p_cam_transform) {
	Transform transform_for_eye;

	ARVRServer *arvr_server = ARVRServer::get_singleton();
	ERR_FAIL_NULL_V(arvr_server, transform_for_eye);

	// @todo In 3DOF where we only have rotation, we should take the position from the p_cam_transform, I think?

	float *js_matrix = godot_webxr_get_transform_for_eye(p_eye);
	if (!initialized || js_matrix == nullptr) {
		transform_for_eye = p_cam_transform;
		return transform_for_eye;
	}

	transform_for_eye = _js_matrix_to_transform(js_matrix);
	free(js_matrix);

	return p_cam_transform * arvr_server->get_reference_frame() * transform_for_eye;
};

CameraMatrix WebXRInterface::get_projection_for_eye(ARVRInterface::Eyes p_eye, real_t p_aspect, real_t p_z_near, real_t p_z_far) {
	CameraMatrix eye;

	float *js_matrix = godot_webxr_get_projection_for_eye(p_eye);
	if (!initialized || js_matrix == nullptr) {
		return eye;
	}

	int k = 0;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			eye.matrix[i][j] = js_matrix[k++];
		}
	}

	free(js_matrix);

	// Copied from godot_oculus_mobile's ovr_mobile_session.cpp
	//eye.matrix[2][2] = -(p_z_far + p_z_near) / (p_z_far - p_z_near);
	//eye.matrix[2][3] = -(2.0f * p_z_far * p_z_near) / (p_z_far - p_z_near);

	return eye;
}

unsigned int WebXRInterface::get_external_texture_for_eye(ARVRInterface::Eyes p_eye) {
	if (!initialized) {
		return 0;
	}
	return godot_webxr_get_external_texture_for_eye(p_eye);
}

void WebXRInterface::commit_for_eye(ARVRInterface::Eyes p_eye, RID p_render_target, const Rect2 &p_screen_rect) {
	if (!initialized) {
		return;
	}
	godot_webxr_commit_for_eye(p_eye);
};

void WebXRInterface::process() {
	if (initialized) {
		// @todo This is so much data to pass back - we should really be using some kind of struct!
		int *tracker_data = godot_webxr_get_tracker_data();
		if (tracker_data == nullptr) {
			return;
		}

		int tracker_count = tracker_data[0];
		if (tracker_count == 0) {
			return;
		}

		float *tracker_matrices = (float *)(tracker_data + 1);
		for (int i = 0; i < tracker_count; i++) {
			_update_tracker(i + 1, _js_matrix_to_transform(tracker_matrices + (i * 16)));
		}

		free(tracker_data);
	};
};

void WebXRInterface::_update_tracker(int p_tracker_id, Transform p_transform) {
	ARVRServer *arvr_server = ARVRServer::get_singleton();
	ERR_FAIL_NULL(arvr_server);

	ARVRPositionalTracker *tracker = arvr_server->find_by_type_and_id(ARVRServer::TRACKER_CONTROLLER, p_tracker_id);
	if (tracker == nullptr) {
		tracker = memnew(ARVRPositionalTracker);
		tracker->set_name(p_tracker_id == 1 ? "Left" : "Right");
		tracker->set_type(ARVRServer::TRACKER_CONTROLLER);
		tracker->set_hand(p_tracker_id == 1 ? ARVRPositionalTracker::TRACKER_LEFT_HAND : ARVRPositionalTracker::TRACKER_RIGHT_HAND);
		// Unfortunately, we can't just use tracker->set_joy_id() because WebXR gamepads aren't
		// registered with the normal Gamepad API per the WebXR spec.
		//
		// See: https://immersive-web.github.io/webxr-gamepads-module/#navigator-differences
		arvr_server->add_tracker(tracker);
	}

	tracker->set_position(p_transform.origin);
	tracker->set_orientation(p_transform.basis);
}

void WebXRInterface::notification(int p_what) {
	// nothing to do here, I guess we could pauze our sensors...
}

WebXRInterface::WebXRInterface() {
	initialized = false;
	session_mode = "inline";
	requested_reference_space_types = "local";
};

WebXRInterface::~WebXRInterface() {
	// and make sure we cleanup if we haven't already
	if (initialized) {
		uninitialize();
	};
};

#endif // JAVASCRIPT_ENABLED
