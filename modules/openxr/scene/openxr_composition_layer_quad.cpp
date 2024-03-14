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

#include "scene/3d/mesh_instance_3d.h"
#include "scene/main/viewport.h"
#include "scene/resources/3d/primitive_meshes.h"
#include "servers/rendering/rendering_server_default.h"
#include "servers/rendering_server.h"

OpenXRCompositionLayerQuad::OpenXRCompositionLayerQuad() {
	composition_layer = {
		XR_TYPE_COMPOSITION_LAYER_QUAD, // type
		nullptr, // next
		0, // flags
		XR_NULL_HANDLE, // space
		XR_EYE_VISIBILITY_BOTH, // eyeVisibility
		{}, // subImage
		{ { 0, 0, 0, 0 }, { 0, 0, 0 } }, // pose
		{ size.x, size.y }, // size
	};
	openxr_layer_provider = memnew(OpenXRViewportCompositionLayerProvider((XrCompositionLayerBaseHeader *)&composition_layer));

	openxr_api = OpenXRAPI::get_singleton();
	if (openxr_api != nullptr) {
		set_process_internal(true);
		set_notify_local_transform(true);
	}

	if (Engine::get_singleton()->is_editor_hint()) {
		fallback = memnew(MeshInstance3D);

		Ref<QuadMesh> mesh;
		mesh.instantiate();
		fallback->set_mesh(mesh);

		Ref<StandardMaterial3D> material;
		material.instantiate();
		fallback->set_surface_override_material(0, material);

		add_child(fallback, false, INTERNAL_MODE_FRONT);
	}
}

OpenXRCompositionLayerQuad::~OpenXRCompositionLayerQuad() {
	if (openxr_layer_provider != nullptr) {
		memdelete(openxr_layer_provider);
		openxr_layer_provider = nullptr;
	}
}

void OpenXRCompositionLayerQuad::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_size", "size"), &OpenXRCompositionLayerQuad::set_size);
	ClassDB::bind_method(D_METHOD("get_size"), &OpenXRCompositionLayerQuad::get_size);

	ClassDB::bind_method(D_METHOD("set_viewport", "viewport"), &OpenXRCompositionLayerQuad::set_viewport);
	ClassDB::bind_method(D_METHOD("get_viewport"), &OpenXRCompositionLayerQuad::get_viewport);

	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "size", PROPERTY_HINT_NONE, ""), "set_size", "get_size");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "viewport", PROPERTY_HINT_NODE_TYPE, "SubViewport"), "set_viewport", "get_viewport");
}

void OpenXRCompositionLayerQuad::set_viewport(SubViewport *p_viewport) {
	RenderingServer *rs = RenderingServer::get_singleton();
	ERR_FAIL_NULL(rs);

	if (viewport != p_viewport) {
		if (viewport) {
			RID vp = viewport->get_viewport_rid();
			RID rt = rs->viewport_get_render_target(vp);
			RSG::texture_storage->render_target_set_override(rt, RID(), RID(), RID());
		}

		viewport = p_viewport;
	}
}

SubViewport *OpenXRCompositionLayerQuad::get_viewport() const {
	return viewport;
}

void OpenXRCompositionLayerQuad::_notification(int p_what) {
	if (openxr_api == nullptr) {
		return;
	}

	RenderingServer *rs = RenderingServer::get_singleton();
	ERR_FAIL_NULL(rs);

	switch (p_what) {
		case NOTIFICATION_INTERNAL_PROCESS: {
			if (viewport) {
				// TODO, if our update mode will result in us not updating our viewport,
				// we should skip this and reuse our last result.

				// Update our XR swapchain
				Size2i vp_size = viewport->get_size();
				openxr_layer_provider->update_swapchain(vp_size.width, vp_size.height);

				// Render to our XR swapchain image
				RID vp = viewport->get_viewport_rid();
				RID rt = rs->viewport_get_render_target(vp);
				RSG::texture_storage->render_target_set_override(rt, openxr_layer_provider->get_image(), RID(), RID());
			}
		} break;
		case NOTIFICATION_LOCAL_TRANSFORM_CHANGED: {
			Transform3D transform = get_transform();
			Quaternion quat(transform.basis);
			composition_layer.pose.orientation = { quat.x, quat.y, quat.z, quat.w };
			composition_layer.pose.position = { transform.origin.x, transform.origin.y, transform.origin.z };
		} break;
		case NOTIFICATION_ENTER_TREE: {
			// Unregister our composition layer provider to our OpenXR API.
			openxr_api->register_composition_layer_provider(openxr_layer_provider);
		} break;
		case NOTIFICATION_EXIT_TREE: {
			// Unregister our composition layer provider.
			openxr_api->unregister_composition_layer_provider(openxr_layer_provider);

			// Reset our viewport.
			if (viewport) {
				RID vp = viewport->get_viewport_rid();
				RID rt = rs->viewport_get_render_target(vp);
				RSG::texture_storage->render_target_set_override(rt, RID(), RID(), RID());
			}

			// Free our swap chain.
			openxr_layer_provider->free_swapchain();
		} break;
	}
}
