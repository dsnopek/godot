/*************************************************************************/
/*  webxr_interface_js.cpp                                               */
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

#ifdef WEB_ENABLED

#include "webxr_interface_js.h"

#include "core/input/input.h"
#include "core/os/os.h"
#include "drivers/gles3/storage/texture_storage.h"
#include "emscripten.h"
#include "godot_webxr.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "servers/rendering/renderer_compositor.h"
#include "servers/rendering/rendering_server_globals.h"

#include <stdlib.h>

void _emwebxr_on_session_supported(char *p_session_mode, int p_supported) {
	XRServer *xr_server = XRServer::get_singleton();
	ERR_FAIL_NULL(xr_server);

	Ref<XRInterface> interface = xr_server->find_interface("WebXR");
	ERR_FAIL_COND(interface.is_null());

	String session_mode = String(p_session_mode);
	interface->emit_signal(SNAME("session_supported"), session_mode, p_supported ? true : false);
}

void _emwebxr_on_session_started(char *p_reference_space_type) {
	XRServer *xr_server = XRServer::get_singleton();
	ERR_FAIL_NULL(xr_server);

	Ref<XRInterface> interface = xr_server->find_interface("WebXR");
	ERR_FAIL_COND(interface.is_null());

	String reference_space_type = String(p_reference_space_type);
	static_cast<WebXRInterfaceJS *>(interface.ptr())->_set_reference_space_type(reference_space_type);
	interface->emit_signal(SNAME("session_started"));
}

void _emwebxr_on_session_ended() {
	XRServer *xr_server = XRServer::get_singleton();
	ERR_FAIL_NULL(xr_server);

	Ref<XRInterface> interface = xr_server->find_interface("WebXR");
	ERR_FAIL_COND(interface.is_null());

	interface->uninitialize();
	interface->emit_signal(SNAME("session_ended"));
}

void _emwebxr_on_session_failed(char *p_message) {
	XRServer *xr_server = XRServer::get_singleton();
	ERR_FAIL_NULL(xr_server);

	Ref<XRInterface> interface = xr_server->find_interface("WebXR");
	ERR_FAIL_COND(interface.is_null());

	interface->uninitialize();

	String message = String(p_message);
	interface->emit_signal(SNAME("session_failed"), message);
}

void _emwebxr_on_controller_changed() {
	XRServer *xr_server = XRServer::get_singleton();
	ERR_FAIL_NULL(xr_server);

	Ref<XRInterface> interface = xr_server->find_interface("WebXR");
	ERR_FAIL_COND(interface.is_null());

	static_cast<WebXRInterfaceJS *>(interface.ptr())->_on_controller_changed();
}

extern "C" EMSCRIPTEN_KEEPALIVE void _emwebxr_on_input_event(int p_event_type, int p_input_source) {
	XRServer *xr_server = XRServer::get_singleton();
	ERR_FAIL_NULL(xr_server);

	Ref<XRInterface> interface = xr_server->find_interface("WebXR");
	ERR_FAIL_COND(interface.is_null());

	((WebXRInterfaceJS *)interface.ptr())->_on_input_event(p_event_type, p_input_source);
}

extern "C" EMSCRIPTEN_KEEPALIVE void _emwebxr_on_simple_event(char *p_signal_name) {
	XRServer *xr_server = XRServer::get_singleton();
	ERR_FAIL_NULL(xr_server);

	Ref<XRInterface> interface = xr_server->find_interface("WebXR");
	ERR_FAIL_COND(interface.is_null());

	StringName signal_name = StringName(p_signal_name);
	interface->emit_signal(signal_name);
}

void WebXRInterfaceJS::is_session_supported(const String &p_session_mode) {
	godot_webxr_is_session_supported(p_session_mode.utf8().get_data(), &_emwebxr_on_session_supported);
}

void WebXRInterfaceJS::set_session_mode(String p_session_mode) {
	session_mode = p_session_mode;
}

String WebXRInterfaceJS::get_session_mode() const {
	return session_mode;
}

void WebXRInterfaceJS::set_required_features(String p_required_features) {
	required_features = p_required_features;
}

String WebXRInterfaceJS::get_required_features() const {
	return required_features;
}

void WebXRInterfaceJS::set_optional_features(String p_optional_features) {
	optional_features = p_optional_features;
}

String WebXRInterfaceJS::get_optional_features() const {
	return optional_features;
}

void WebXRInterfaceJS::set_requested_reference_space_types(String p_requested_reference_space_types) {
	requested_reference_space_types = p_requested_reference_space_types;
}

String WebXRInterfaceJS::get_requested_reference_space_types() const {
	return requested_reference_space_types;
}

void WebXRInterfaceJS::_set_reference_space_type(String p_reference_space_type) {
	reference_space_type = p_reference_space_type;
}

String WebXRInterfaceJS::get_reference_space_type() const {
	return reference_space_type;
}

bool WebXRInterfaceJS::is_input_source_active(int p_input_source_id) const {
	ERR_FAIL_INDEX_V(p_input_source_id, input_source_count, false);
	return input_sources[p_input_source_id].active;
}

Ref<XRPositionalTracker> WebXRInterfaceJS::get_input_source_tracker(int p_input_source_id) const {
	ERR_FAIL_INDEX_V(p_input_source_id, input_source_count, Ref<XRPositionalTracker>());
	return input_sources[p_input_source_id].tracker;
}

WebXRInterface::TargetRayMode WebXRInterfaceJS::get_input_source_target_ray_mode(int p_input_source_id) const {
	ERR_FAIL_INDEX_V(p_input_source_id, input_source_count, WebXRInterface::TARGET_RAY_MODE_UNKNOWN);
	if (!input_sources[p_input_source_id].active) {
		return WebXRInterface::TARGET_RAY_MODE_UNKNOWN;
	}
	return input_sources[p_input_source_id].target_ray_mode;
}

String WebXRInterfaceJS::get_visibility_state() const {
	char *c_str = godot_webxr_get_visibility_state();
	if (c_str) {
		String visibility_state = String(c_str);
		free(c_str);

		return visibility_state;
	}
	return String();
}

PackedVector3Array WebXRInterfaceJS::get_bounds_geometry() const {
	PackedVector3Array ret;

	int *js_bounds = godot_webxr_get_bounds_geometry();
	if (js_bounds) {
		ret.resize(js_bounds[0]);
		for (int i = 0; i < js_bounds[0]; i++) {
			float *js_vector3 = ((float *)js_bounds) + (i * 3) + 1;
			ret.set(i, Vector3(js_vector3[0], js_vector3[1], js_vector3[2]));
		}
		free(js_bounds);
	}

	return ret;
}

StringName WebXRInterfaceJS::get_name() const {
	return "WebXR";
};

uint32_t WebXRInterfaceJS::get_capabilities() const {
	return XRInterface::XR_STEREO | XRInterface::XR_MONO;
};

uint32_t WebXRInterfaceJS::get_view_count() {
	int view_count = godot_webxr_get_view_count();
	return view_count >= 1 ? view_count : 1;
};

bool WebXRInterfaceJS::is_initialized() const {
	return (initialized);
};

bool WebXRInterfaceJS::initialize() {
	XRServer *xr_server = XRServer::get_singleton();
	ERR_FAIL_NULL_V(xr_server, false);

	if (!initialized) {
		if (!godot_webxr_is_supported()) {
			return false;
		}

		if (requested_reference_space_types.size() == 0) {
			return false;
		}

		// we must create a tracker for our head
		head_transform.basis = Basis();
		head_transform.origin = Vector3();
		head_tracker.instantiate();
		head_tracker->set_tracker_type(XRServer::TRACKER_HEAD);
		head_tracker->set_tracker_name("head");
		head_tracker->set_tracker_desc("Players head");
		xr_server->add_tracker(head_tracker);

		// make this our primary interface
		xr_server->set_primary_interface(this);

		// Clear state variables.
		memset(touches, 0, sizeof touches);

		// Clear render_targetsize to make sure it gets reset to the new size.
		// Clearing in uninitialize() doesn't work because a frame can still be
		// rendered after it's called, which will fill render_targetsize again.
		render_targetsize.width = 0;
		render_targetsize.height = 0;

		initialized = true;

		godot_webxr_initialize(
				session_mode.utf8().get_data(),
				required_features.utf8().get_data(),
				optional_features.utf8().get_data(),
				requested_reference_space_types.utf8().get_data(),
				&_emwebxr_on_session_started,
				&_emwebxr_on_session_ended,
				&_emwebxr_on_session_failed,
				&_emwebxr_on_controller_changed,
				&_emwebxr_on_input_event,
				&_emwebxr_on_simple_event);
	};

	return true;
};

void WebXRInterfaceJS::uninitialize() {
	if (initialized) {
		XRServer *xr_server = XRServer::get_singleton();
		if (xr_server != nullptr) {
			if (head_tracker.is_valid()) {
				xr_server->remove_tracker(head_tracker);

				head_tracker.unref();
			}

			if (xr_server->get_primary_interface() == this) {
				// no longer our primary interface
				xr_server->set_primary_interface(nullptr);
			}
		}

		godot_webxr_uninitialize();

		GLES3::TextureStorage *texture_storage = dynamic_cast<GLES3::TextureStorage *>(RSG::texture_storage);
		if (texture_storage != nullptr) {
			for (KeyValue<unsigned int, RID> &E : texture_cache) {
				// Forcibly mark as not part of a render target so we can free it.
				GLES3::Texture *texture = texture_storage->get_texture(E.value);
				texture->is_render_target = false;

				texture_storage->texture_free(E.value);
			}
		}

		texture_cache.clear();
		reference_space_type = "";
		initialized = false;
	};
};

Transform3D WebXRInterfaceJS::_js_matrix_to_transform(float *p_js_matrix) {
	Transform3D transform;

	transform.basis.rows[0].x = p_js_matrix[0];
	transform.basis.rows[1].x = p_js_matrix[1];
	transform.basis.rows[2].x = p_js_matrix[2];
	transform.basis.rows[0].y = p_js_matrix[4];
	transform.basis.rows[1].y = p_js_matrix[5];
	transform.basis.rows[2].y = p_js_matrix[6];
	transform.basis.rows[0].z = p_js_matrix[8];
	transform.basis.rows[1].z = p_js_matrix[9];
	transform.basis.rows[2].z = p_js_matrix[10];
	transform.origin.x = p_js_matrix[12];
	transform.origin.y = p_js_matrix[13];
	transform.origin.z = p_js_matrix[14];

	return transform;
}

Size2 WebXRInterfaceJS::get_render_target_size() {
	if (render_targetsize.width != 0 && render_targetsize.height != 0) {
		return render_targetsize;
	}

	int *js_size = godot_webxr_get_render_target_size();
	if (!initialized || js_size == nullptr) {
		// As a temporary default (until WebXR is fully initialized), use the
		// window size.
		return DisplayServer::get_singleton()->window_get_size();
	}

	render_targetsize.width = js_size[0];
	render_targetsize.height = js_size[1];

	free(js_size);

	return render_targetsize;
};

Transform3D WebXRInterfaceJS::get_camera_transform() {
	Transform3D transform_for_eye;

	XRServer *xr_server = XRServer::get_singleton();
	ERR_FAIL_NULL_V(xr_server, transform_for_eye);

	if (initialized) {
		float world_scale = xr_server->get_world_scale();

		// just scale our origin point of our transform
		Transform3D _head_transform = head_transform;
		_head_transform.origin *= world_scale;

		transform_for_eye = (xr_server->get_reference_frame()) * _head_transform;
	}

	return transform_for_eye;
};

Transform3D WebXRInterfaceJS::get_transform_for_view(uint32_t p_view, const Transform3D &p_cam_transform) {
	Transform3D transform_for_eye;

	XRServer *xr_server = XRServer::get_singleton();
	ERR_FAIL_NULL_V(xr_server, transform_for_eye);

	float *js_matrix = godot_webxr_get_transform_for_eye(p_view + 1);
	if (!initialized || js_matrix == nullptr) {
		transform_for_eye = p_cam_transform;
		return transform_for_eye;
	}

	transform_for_eye = _js_matrix_to_transform(js_matrix);
	free(js_matrix);

	float world_scale = xr_server->get_world_scale();
	// Scale only the center point of our eye transform, so we don't scale the
	// distance between the eyes.
	Transform3D _head_transform = head_transform;
	transform_for_eye.origin -= _head_transform.origin;
	_head_transform.origin *= world_scale;
	transform_for_eye.origin += _head_transform.origin;

	return p_cam_transform * xr_server->get_reference_frame() * transform_for_eye;
};

Projection WebXRInterfaceJS::get_projection_for_view(uint32_t p_view, double p_aspect, double p_z_near, double p_z_far) {
	Projection eye;

	float *js_matrix = godot_webxr_get_projection_for_eye(p_view + 1);
	if (!initialized || js_matrix == nullptr) {
		return eye;
	}

	int k = 0;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			eye.columns[i][j] = js_matrix[k++];
		}
	}

	free(js_matrix);

	// Copied from godot_oculus_mobile's ovr_mobile_session.cpp
	eye.columns[2][2] = -(p_z_far + p_z_near) / (p_z_far - p_z_near);
	eye.columns[3][2] = -(2.0f * p_z_far * p_z_near) / (p_z_far - p_z_near);

	return eye;
}

bool WebXRInterfaceJS::pre_draw_viewport(RID p_render_target) {
	GLES3::TextureStorage *texture_storage = dynamic_cast<GLES3::TextureStorage *>(RSG::texture_storage);
	if (texture_storage == nullptr) {
		return false;
	}

	GLES3::RenderTarget *rt = texture_storage->get_render_target(p_render_target);
	if (rt == nullptr) {
		return false;
	}

	RID color_texture = get_color_texture();
	RID depth_texture = get_depth_texture();

	// If the texture resources don't change, we need to re-attach the color and
	// depth textures every frame. WebXR is doing something tricky where the
	// 'texture' object is the same, but it changes internally to move to the
	// next buffer in the swap chain.
	if (rt->overridden.is_overridden && rt->overridden.color == color_texture && rt->overridden.depth == depth_texture) {
		GLES3::Config *config = GLES3::Config::get_singleton();
		bool use_multiview = rt->view_count > 1 && config->multiview_supported;

		glBindFramebuffer(GL_FRAMEBUFFER, rt->fbo);
		if (use_multiview) {
			glFramebufferTextureMultiviewOVR(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, rt->color, 0, 0, rt->view_count);
			glFramebufferTextureMultiviewOVR(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, rt->depth, 0, 0, rt->view_count);
		} else {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt->color, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, rt->depth, 0);
		}
		glBindFramebuffer(GL_FRAMEBUFFER, texture_storage->system_fbo);
	}

	return true;
}

Vector<BlitToScreen> WebXRInterfaceJS::post_draw_viewport(RID p_render_target, const Rect2 &p_screen_rect) {
	Vector<BlitToScreen> blit_to_screen;

	// We don't need to do anything here.

	return blit_to_screen;
};

RID WebXRInterfaceJS::_get_texture(unsigned int p_texture_id) {
	RBMap<unsigned int, RID>::Element *cache = texture_cache.find(p_texture_id);
	if (cache != nullptr) {
		return cache->get();
	}

	GLES3::TextureStorage *texture_storage = dynamic_cast<GLES3::TextureStorage *>(RSG::texture_storage);
	if (texture_storage == nullptr) {
		return RID();
	}

	uint32_t view_count = get_view_count();
	Size2 texture_size = get_render_target_size();

	RID texture = texture_storage->texture_create_external(
			view_count == 1 ? GLES3::Texture::TYPE_2D : GLES3::Texture::TYPE_LAYERED,
			Image::FORMAT_RGBA8,
			p_texture_id,
			(int)texture_size.width,
			(int)texture_size.height,
			1,
			view_count);

	texture_cache.insert(p_texture_id, texture);

	return texture;
}

RID WebXRInterfaceJS::get_color_texture() {
	unsigned int texture_id = godot_webxr_get_color_texture();
	if (texture_id == 0) {
		return RID();
	}

	return _get_texture(texture_id);
}

RID WebXRInterfaceJS::get_depth_texture() {
	unsigned int texture_id = godot_webxr_get_depth_texture();
	if (texture_id == 0) {
		return RID();
	}

	return _get_texture(texture_id);
}

RID WebXRInterfaceJS::get_velocity_texture() {
	unsigned int texture_id = godot_webxr_get_velocity_texture();
	if (texture_id == 0) {
		return RID();
	}

	return _get_texture(texture_id);
}

void WebXRInterfaceJS::process() {
	if (initialized) {
		// Get the "head" position.
		float *js_matrix = godot_webxr_get_transform_for_eye(0);
		if (js_matrix != nullptr) {
			head_transform = _js_matrix_to_transform(js_matrix);
			free(js_matrix);
		}
		if (head_tracker.is_valid()) {
			head_tracker->set_pose("default", head_transform, Vector3(), Vector3());
		}

		for (int i = 0; i < input_source_count; i++) {
			_update_input_source(i);
		}
	};
};

void WebXRInterfaceJS::_update_input_source(int p_input_source_id) {
	WebXRInterface::TargetRayMode target_ray_mode = (WebXRInterface::TargetRayMode)godot_webxr_get_controller_target_ray_mode(p_controller_id);

	if (target_ray_mode == WebXRInterface::TARGET_RAY_MODE_SCREEN) {
		int touch_index = _get_touch_index(p_controller_id);
		if (touch_index < 5 && touches[touch_index].is_touching) {
			int *axes = godot_webxr_get_controller_axes(p_controller_id);
			if (axes) {
				// @todo Combine these into a single function call?
				Vector2 joy_vector = _get_joy_vector_from_axes(axes);
				Vector2 position = _get_screen_position_from_joy_vector(joy_vector);
				Vector2 delta = position - touches[touch_index].previous_position;

				// If position has changed by at least 1 pixel, generate a drag event.
				if (abs(delta.x) >= 1.0 || abs(delta.y) >= 1.0) {
					Ref<InputEventScreenDrag> event;
					event.instantiate();
					event->set_index(touch_index);
					event->set_position(position);
					event->set_relative(delta);
					Input::get_singleton()->parse_input_event(event);

					touches[touch_index].previous_position = position;
				}
			}
			free(axes);
		}
		return;
	}

	XRServer *xr_server = XRServer::get_singleton();
	ERR_FAIL_NULL(xr_server);

	StringName tracker_name;
	switch (p_controller_id) {
		case 0: {
			tracker_name = SNAME("left_hand");
		} break;

		case 1: {
			tracker_name = SNAME("right_hand");
		} break;

		default: {
			char tmp[16];
			sprintf(tmp, "tracker_%i", p_controller_id);
			tracker_name = tmp;
		}
	}

	Ref<XRPositionalTracker> tracker = xr_server->get_tracker(tracker_name);
	if (godot_webxr_is_controller_connected(p_controller_id)) {
		if (tracker.is_null()) {
			tracker.instantiate();
			// Controller id's 0 and 1 are always the left and right hands.
			if (p_controller_id < 2) {
				tracker->set_tracker_type(XRServer::TRACKER_CONTROLLER);
				tracker->set_tracker_name(tracker_name);
				tracker->set_tracker_desc(p_controller_id == 0 ? "Left hand controller" : "Right hand controller");
				tracker->set_tracker_hand(p_controller_id == 0 ? XRPositionalTracker::TRACKER_HAND_LEFT : XRPositionalTracker::TRACKER_HAND_RIGHT);
			} else {
				tracker->set_tracker_name(tracker_name);
				tracker->set_tracker_desc(tracker_name);
			}
			xr_server->add_tracker(tracker);
		}

		float *tracker_matrix = godot_webxr_get_controller_transform(p_controller_id);
		if (tracker_matrix) {
			// Note, poses should NOT have world scale and our reference frame applied!
			Transform3D transform = _js_matrix_to_transform(tracker_matrix);
			tracker->set_pose("default", transform, Vector3(), Vector3());
			free(tracker_matrix);
		}

		// TODO implement additional poses such as "aim" and "grip"

		int *buttons = godot_webxr_get_controller_buttons(p_controller_id);
		if (buttons) {
			// TODO buttons should be named properly, this is just a temporary fix
			for (int i = 0; i < buttons[0]; i++) {
				char name[1024];
				sprintf(name, "button_%i", i);

				float value = *((float *)buttons + (i + 1));
				bool state = value > 0.0;
				tracker->set_input(name, state);
			}
			free(buttons);
		}

		int *axes = godot_webxr_get_controller_axes(p_controller_id);
		if (axes) {
			// TODO again just a temporary fix, split these between proper float and vector2 inputs
			for (int i = 0; i < axes[0]; i++) {
				char name[1024];
				sprintf(name, "axis_%i", i);

				float value = *((float *)axes + (i + 1));
				tracker->set_input(name, value);
			}
			free(axes);
		}
	} else if (tracker.is_valid()) {
		xr_server->remove_tracker(tracker);
		tracker.unref();
	}
}

void WebXRInterfaceJS::_on_controller_changed() {
	// Register "virtual" gamepads with Godot for the ones we get from WebXR.
	/*
	godot_webxr_sample_controller_data();
	for (int i = 0; i < 2; i++) {
		bool controller_connected = godot_webxr_is_controller_connected(i);
		if (controllers_state[i] != controller_connected) {
			controllers_state[i] = controller_connected;
		}
	}
	*/
}

void WebXRInterfaceJS::_on_input_event(int p_event_type, int p_input_source) {
	XRServer *xr_server = XRServer::get_singleton();
	ERR_FAIL_NULL(xr_server);

	Input *input = Input::get_singleton();

	if (p_event_type == WEBXR_INPUT_EVENT_SELECTSTART || p_event_type == WEBXR_INPUT_EVENT_SELECTEND) {
		int target_ray_mode = godot_webxr_get_controller_target_ray_mode(p_input_source);
		if (target_ray_mode == WebXRInterface::TARGET_RAY_MODE_SCREEN) {
			int touch_index = _get_touch_index(p_input_source);
			if (touch_index < 5) {
				touches[touch_index].is_touching = (p_event_type == WEBXR_INPUT_EVENT_SELECTSTART);

				int *axes = godot_webxr_get_controller_axes(p_input_source);
				if (axes) {
					// @todo Combine these into a single function call?
					Vector2 joy_vector = _get_joy_vector_from_axes(axes);
					Vector2 position = _get_screen_position_from_joy_vector(joy_vector);

					Ref<InputEventScreenTouch> event;
					event.instantiate();
					event->set_index(touch_index);
					event->set_position(position);
					event->set_pressed(p_event_type == WEBXR_INPUT_EVENT_SELECTSTART);
					input->parse_input_event(event);

					touches[touch_index].previous_position = position;
				}
			}
		}
	}

	// @todo Don't use controller id for this! Use the tracker? Or, set input on the tracker?
	int controller_id = p_input_source + 1;
	switch (p_event_type) {
		case WEBXR_INPUT_EVENT_SELECTSTART:
			emit_signal("selectstart", controller_id);
			break;

		case WEBXR_INPUT_EVENT_SELECTEND:
			emit_signal("selectend", controller_id);
			break;

		case WEBXR_INPUT_EVENT_SELECT:
			emit_signal("select", controller_id);
			break;

		case WEBXR_INPUT_EVENT_SQUEEZESTART:
			emit_signal("squeezestart", controller_id);
			break;

		case WEBXR_INPUT_EVENT_SQUEEZEEND:
			emit_signal("squeezeend", controller_id);
			break;

		case WEBXR_INPUT_EVENT_SQUEEZE:
			emit_signal("squeeze", controller_id);
			break;
	}
}

int WebXRInterfaceJS::_get_touch_index(int p_input_source) {
	int index = 0;
	for (int i = 0; i < p_input_source; i++) {
		if (godot_webxr_get_controller_target_ray_mode(i) == WebXRInterface::TARGET_RAY_MODE_SCREEN) {
			index++;
		}
	}
	return index;
}

Vector2 WebXRInterfaceJS::_get_joy_vector_from_axes(int *p_axes) {
	float x_axis = 0.0;
	float y_axis = 0.0;

	if (p_axes[0] >= 2) {
		x_axis = *((float *)p_axes + 1);
		y_axis = *((float *)p_axes + 2);
	}

	return Vector2(x_axis, y_axis);
}

Vector2 WebXRInterfaceJS::_get_screen_position_from_joy_vector(const Vector2 &p_joy_vector) {
	SceneTree *scene_tree = Object::cast_to<SceneTree>(OS::get_singleton()->get_main_loop());
	if (!scene_tree) {
		return Vector2();
	}

	Window *viewport = scene_tree->get_root();

	// Invert the y-axis.
	Vector2 position_percentage((p_joy_vector.x + 1.0f) / 2.0f, ((-p_joy_vector.y) + 1.0f) / 2.0f);
	Vector2 position = (Size2)viewport->get_size() * position_percentage;

	return position;
}

WebXRInterfaceJS::WebXRInterfaceJS() {
	initialized = false;
	session_mode = "inline";
	requested_reference_space_types = "local";
};

WebXRInterfaceJS::~WebXRInterfaceJS() {
	// and make sure we cleanup if we haven't already
	if (initialized) {
		uninitialize();
	};
};

#endif // WEB_ENABLED
