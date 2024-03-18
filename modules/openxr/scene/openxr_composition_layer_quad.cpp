/**************************************************************************/
/*  openxr_composition_layer_quad.cpp                                     */
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

#include "openxr_composition_layer_quad.h"

#include "../extensions/openxr_composition_layer_extension.h"
#include "../openxr_api.h"
#include "../openxr_interface.h"

#include "scene/3d/mesh_instance_3d.h"
#include "scene/main/viewport.h"
#include "scene/resources/3d/primitive_meshes.h"

OpenXRCompositionLayerQuad::OpenXRCompositionLayerQuad() {
	openxr_api = OpenXRAPI::get_singleton();

	composition_layer = {
		XR_TYPE_COMPOSITION_LAYER_QUAD, // type
		nullptr, // next
		0, // flags
		XR_NULL_HANDLE, // space
		XR_EYE_VISIBILITY_BOTH, // eyeVisibility
		{}, // subImage
		{ { 0, 0, 0, 0 }, { 0, 0, 0 } }, // pose
		{ quad_size.x, quad_size.y }, // size
	};
	openxr_layer_provider = memnew(OpenXRViewportCompositionLayerProvider((XrCompositionLayerBaseHeader *)&composition_layer));

	Ref<OpenXRInterface> openxr_interface = XRServer::get_singleton()->find_interface("OpenXR");
	if (openxr_interface.is_valid()) {
		openxr_interface->connect("session_begun", callable_mp(this, &OpenXRCompositionLayerQuad::_on_openxr_session_begun));
	}

	if (Engine::get_singleton()->is_editor_hint()) {
		fallback = memnew(MeshInstance3D);

		Ref<QuadMesh> mesh;
		mesh.instantiate();
		mesh->set_size(quad_size);
		fallback->set_mesh(mesh);

		Ref<StandardMaterial3D> material;
		material.instantiate();
		material->set_local_to_scene(true);
		fallback->set_surface_override_material(0, material);

		add_child(fallback, false, INTERNAL_MODE_FRONT);
	}

	set_process_internal(true);
	set_notify_local_transform(true);
}

OpenXRCompositionLayerQuad::~OpenXRCompositionLayerQuad() {
	Ref<OpenXRInterface> openxr_interface = XRServer::get_singleton()->find_interface("OpenXR");
	if (openxr_interface.is_valid()) {
		openxr_interface->disconnect("session_begun", callable_mp(this, &OpenXRCompositionLayerQuad::_on_openxr_session_begun));
	}

	if (openxr_layer_provider != nullptr) {
		memdelete(openxr_layer_provider);
		openxr_layer_provider = nullptr;
	}
}

void OpenXRCompositionLayerQuad::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_quad_size", "size"), &OpenXRCompositionLayerQuad::set_quad_size);
	ClassDB::bind_method(D_METHOD("get_quad_size"), &OpenXRCompositionLayerQuad::get_quad_size);

	ClassDB::bind_method(D_METHOD("set_layer_viewport", "viewport"), &OpenXRCompositionLayerQuad::set_layer_viewport);
	ClassDB::bind_method(D_METHOD("get_layer_viewport"), &OpenXRCompositionLayerQuad::get_layer_viewport);

	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "quad_size", PROPERTY_HINT_NONE, ""), "set_quad_size", "get_quad_size");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "layer_viewport", PROPERTY_HINT_NODE_TYPE, "SubViewport"), "set_layer_viewport", "get_layer_viewport");
}

void OpenXRCompositionLayerQuad::_on_openxr_session_begun() {
	if (openxr_api) {
		composition_layer.space = openxr_api->get_play_space();
	}
}

void OpenXRCompositionLayerQuad::set_quad_size(const Size2 &p_size) {
	quad_size = p_size;

	composition_layer.size = { quad_size.x, quad_size.y };

	if (fallback) {
		Ref<QuadMesh> mesh = fallback->get_mesh();
		if (mesh.is_valid()) {
			mesh->set_size(p_size);
		}
	}
}

Size2 OpenXRCompositionLayerQuad::get_quad_size() const {
	return quad_size;
}

void OpenXRCompositionLayerQuad::set_layer_viewport(SubViewport *p_viewport) {
	layer_viewport = p_viewport;
	if (is_visible() && is_inside_tree() && openxr_layer_provider->set_viewport(p_viewport)) {
		if (layer_viewport && fallback) {
			_reset_fallback_material();
		}
	}
}

SubViewport *OpenXRCompositionLayerQuad::get_layer_viewport() const {
	return layer_viewport;
}

void OpenXRCompositionLayerQuad::_reset_fallback_material() {
	ERR_FAIL_COND(!fallback);
	ERR_FAIL_COND(!layer_viewport);

	Ref<StandardMaterial3D> material = fallback->get_surface_override_material(0);
	if (material.is_valid()) {
		Node *loc_scene = material->get_local_scene();
		if (loc_scene) {
			Ref<ViewportTexture> texture = material->get_texture(StandardMaterial3D::TEXTURE_ALBEDO);
			if (texture.is_null()) {
				texture.instantiate();
				material->set_texture(StandardMaterial3D::TEXTURE_ALBEDO, texture);
			}
			NodePath viewport_path = loc_scene->get_path_to(layer_viewport);
			texture->set_viewport_path_in_scene(viewport_path);
		}
	}
}

void OpenXRCompositionLayerQuad::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			if (layer_viewport && fallback) {
				_reset_fallback_material();
			}
		} break;
		case NOTIFICATION_INTERNAL_PROCESS: {
			if (is_visible()) {
				openxr_layer_provider->process();
			}
		} break;
		case NOTIFICATION_LOCAL_TRANSFORM_CHANGED: {
			Transform3D transform = get_transform();
			Quaternion quat(transform.basis);
			composition_layer.pose.orientation = { quat.x, quat.y, quat.z, quat.w };
			composition_layer.pose.position = { transform.origin.x, transform.origin.y, transform.origin.z };
		} break;
		case NOTIFICATION_VISIBILITY_CHANGED: {
			if (is_inside_tree()) {
				openxr_layer_provider->set_viewport(is_visible() ? layer_viewport : nullptr);
			}
		} break;
		case NOTIFICATION_ENTER_TREE: {
			if (openxr_api) {
				// Register our composition layer provider to our OpenXR API.
				openxr_api->register_composition_layer_provider(openxr_layer_provider);
			}
			if (layer_viewport) {
				openxr_layer_provider->set_viewport(is_visible() ? layer_viewport : nullptr);
			}
		} break;
		case NOTIFICATION_EXIT_TREE: {
			if (openxr_api) {
				// Unregister our composition layer provider.
				openxr_api->unregister_composition_layer_provider(openxr_layer_provider);
			}
			// This will clean up existing resources.
			openxr_layer_provider->set_viewport(nullptr);
		} break;
	}
}
