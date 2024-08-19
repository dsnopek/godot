/**************************************************************************/
/*  openxr_composition_layer_extension.cpp                                */
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

#include "openxr_composition_layer_extension.h"

#include "servers/rendering/rendering_server_globals.h"

////////////////////////////////////////////////////////////////////////////
// OpenXRCompositionLayerExtension

OpenXRCompositionLayerExtension *OpenXRCompositionLayerExtension::singleton = nullptr;

OpenXRCompositionLayerExtension *OpenXRCompositionLayerExtension::get_singleton() {
	return singleton;
}

OpenXRCompositionLayerExtension::OpenXRCompositionLayerExtension() {
	singleton = this;
}

OpenXRCompositionLayerExtension::~OpenXRCompositionLayerExtension() {
	singleton = nullptr;
}

HashMap<String, bool *> OpenXRCompositionLayerExtension::get_requested_extensions() {
	HashMap<String, bool *> request_extensions;

	request_extensions[XR_KHR_COMPOSITION_LAYER_CYLINDER_EXTENSION_NAME] = &cylinder_ext_available;
	request_extensions[XR_KHR_COMPOSITION_LAYER_EQUIRECT2_EXTENSION_NAME] = &equirect_ext_available;

#ifdef ANDROID_ENABLED
	request_extensions[XR_KHR_ANDROID_SURFACE_SWAPCHAIN_EXTENSION_NAME] = &android_surface_ext_available;
#endif

	return request_extensions;
}

void OpenXRCompositionLayerExtension::on_instance_created(const XrInstance p_instance) {
	EXT_INIT_XR_FUNC(xrDestroySwapchain);
}

void OpenXRCompositionLayerExtension::on_session_created(const XrSession p_session) {
	OpenXRAPI::get_singleton()->register_composition_layer_provider(this);
}

void OpenXRCompositionLayerExtension::on_session_destroyed() {
	OpenXRAPI::get_singleton()->unregister_composition_layer_provider(this);

#ifdef ANDROID_ENABLED
	free_queued_android_surface_swapchains();
#endif
}

void OpenXRCompositionLayerExtension::on_pre_render() {
#ifdef ANDROID_ENABLED
	free_queued_android_surface_swapchains();
#endif

	for (OpenXRViewportCompositionLayerProviderBase *composition_layer : composition_layers) {
		composition_layer->on_pre_render();
	}
}

int OpenXRCompositionLayerExtension::get_composition_layer_count() {
	return composition_layers.size();
}

XrCompositionLayerBaseHeader *OpenXRCompositionLayerExtension::get_composition_layer(int p_index) {
	ERR_FAIL_INDEX_V(p_index, composition_layers.size(), nullptr);
	return composition_layers[p_index]->get_composition_layer();
}

int OpenXRCompositionLayerExtension::get_composition_layer_order(int p_index) {
	ERR_FAIL_INDEX_V(p_index, composition_layers.size(), 1);
	return composition_layers[p_index]->get_sort_order();
}

void OpenXRCompositionLayerExtension::register_viewport_composition_layer_provider(OpenXRViewportCompositionLayerProviderBase *p_composition_layer) {
	composition_layers.push_back(p_composition_layer);
}

void OpenXRCompositionLayerExtension::unregister_viewport_composition_layer_provider(OpenXRViewportCompositionLayerProviderBase *p_composition_layer) {
	composition_layers.erase(p_composition_layer);
}

bool OpenXRCompositionLayerExtension::is_available(XrStructureType p_which) {
	switch (p_which) {
		case XR_TYPE_COMPOSITION_LAYER_QUAD: {
			// Doesn't require an extension.
			return true;
		} break;
		case XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR: {
			return cylinder_ext_available;
		} break;
		case XR_TYPE_COMPOSITION_LAYER_EQUIRECT2_KHR: {
			return equirect_ext_available;
		} break;
		default: {
			ERR_PRINT(vformat("Unsupported composition layer type: %s", p_which));
			return false;
		}
	}
}

#ifdef ANDROID_ENABLED
bool OpenXRCompositionLayerExtension::create_android_surface_swapchain(XrSwapchainCreateInfo *p_info, XrSwapchain *r_swapchain, jobject *r_surface) {
	if (android_surface_ext_available) {
		// @todo We need a way to add to the next pointer chain.
		XrResult result = composition_layer_extension->xrCreateSwapchainAndroidSurfaceKHR(openxr_api->get_session(), &p_info, r_swapchain, r_surface);
		if (XR_FAILED(result)) {
			print_line("OpenXR: Failed to create Android surface swapchain [", openxr_api->get_error_string(result), "]");
			return false;
		}

		return true;
	}

	return false;
}

void OpenXRCompositionLayerExtension::free_android_surface_swapchain(XrSwapchain p_swapchain) {
	android_surface_swapchain_free_queue.push_back(p_swapchain);
}

void OpenXRCompositionLayerExtension::free_queued_android_surface_swapchains() {
	for (XrSwapchain swapchain : android_surface_swapchain_free_queue) {
		xrDestroySwapchain(swapchain);
	}
	android_surface_swapchain_free_queue.clear();
}
#endif

////////////////////////////////////////////////////////////////////////////
// OpenXRViewportCompositionLayerProviderBase

OpenXRViewportCompositionLayerProviderBase::OpenXRViewportCompositionLayerProviderBase(XrCompositionLayerBaseHeader *p_composition_layer) {
	composition_layer = p_composition_layer;
	openxr_api = OpenXRAPI::get_singleton();
	composition_layer_extension = OpenXRCompositionLayerExtension::get_singleton();
}

OpenXRViewportCompositionLayerProviderBase::~OpenXRViewportCompositionLayerProviderBase() {
	for (OpenXRExtensionWrapper *extension : OpenXRAPI::get_registered_extension_wrappers()) {
		extension->on_viewport_composition_layer_destroyed(composition_layer);
	}
}

void OpenXRViewportCompositionLayerProviderBase::set_alpha_blend(bool p_alpha_blend) {
	if (alpha_blend != p_alpha_blend) {
		alpha_blend = p_alpha_blend;
		if (alpha_blend) {
			composition_layer->layerFlags |= XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
		} else {
			composition_layer->layerFlags &= ~XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
		}
	}
}

void OpenXRViewportCompositionLayerProviderBase::set_extension_property_values(const Dictionary &p_extension_property_values) {
	extension_property_values = p_extension_property_values;
	extension_property_values_changed = true;
}

XrCompositionLayerBaseHeader *OpenXRViewportCompositionLayerProviderBase::get_composition_layer() {
	if (openxr_api == nullptr || composition_layer_extension == nullptr) {
		// OpenXR not initialized or we're in the editor?
		return nullptr;
	}

	if (!composition_layer_extension->is_available(composition_layer->type)) {
		// Selected type is not supported, ignore our layer.
		return nullptr;
	}

	XrSwapchainSubImage subimage = {
		0, // swapchain
		{ { 0, 0 }, { 0, 0 } }, // imageRect
		0, // imageArrayIndex
	};
	_update_swapchain_sub_image(subimage);

	if (subimage.swapchain == XR_NULL_HANDLE) {
		// Don't have a swapchain to display? Ignore our layer.
		return nullptr;
	}

	// Update the layer struct for the swapchain.
	switch (composition_layer->type) {
		case XR_TYPE_COMPOSITION_LAYER_QUAD: {
			XrCompositionLayerQuad *quad_layer = (XrCompositionLayerQuad *)composition_layer;
			quad_layer->space = openxr_api->get_play_space();
			quad_layer->subImage = subimage;
		} break;

		case XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR: {
			XrCompositionLayerCylinderKHR *cylinder_layer = (XrCompositionLayerCylinderKHR *)composition_layer;
			cylinder_layer->space = openxr_api->get_play_space();
			cylinder_layer->subImage = subimage;
		} break;

		case XR_TYPE_COMPOSITION_LAYER_EQUIRECT2_KHR: {
			XrCompositionLayerEquirect2KHR *equirect_layer = (XrCompositionLayerEquirect2KHR *)composition_layer;
			equirect_layer->space = openxr_api->get_play_space();
			equirect_layer->subImage = subimage;
		} break;

		default: {
			return nullptr;
		} break;
	}

	if (extension_property_values_changed) {
		extension_property_values_changed = false;

		void *next_pointer = nullptr;
		for (OpenXRExtensionWrapper *extension : OpenXRAPI::get_registered_extension_wrappers()) {
			void *np = extension->set_viewport_composition_layer_and_get_next_pointer(composition_layer, extension_property_values, next_pointer);
			if (np) {
				next_pointer = np;
			}
		}
		composition_layer->next = next_pointer;
	}

	return composition_layer;
}

////////////////////////////////////////////////////////////////////////////
// OpenXRViewportCompositionLayerProvider

void OpenXRViewportCompositionLayerProvider::set_viewport(RID p_viewport, Size2i p_size) {
	RenderingServer *rs = RenderingServer::get_singleton();
	ERR_FAIL_NULL(rs);

	if (viewport != p_viewport) {
		if (viewport.is_valid()) {
			RID rt = rs->viewport_get_render_target(viewport);
			RSG::texture_storage->render_target_set_override(rt, RID(), RID(), RID());
		}

		viewport = p_viewport;

		if (viewport.is_valid()) {
			viewport_size = p_size;
		} else {
			free_swapchain();
			viewport_size = Size2i();
		}
	}
}

void OpenXRViewportCompositionLayerProvider::on_pre_render() {
	RenderingServer *rs = RenderingServer::get_singleton();
	ERR_FAIL_NULL(rs);

	if (viewport.is_valid() && openxr_api && openxr_api->is_running()) {
		RS::ViewportUpdateMode update_mode = rs->viewport_get_update_mode(viewport);
		if (update_mode == RS::VIEWPORT_UPDATE_ONCE || update_mode == RS::VIEWPORT_UPDATE_ALWAYS) {
			// Update our XR swapchain
			if (update_and_acquire_swapchain(update_mode == RS::VIEWPORT_UPDATE_ONCE)) {
				// Render to our XR swapchain image.
				RID rt = rs->viewport_get_render_target(viewport);
				RSG::texture_storage->render_target_set_override(rt, get_current_swapchain_texture(), RID(), RID());
			}
		}
	}
}

void OpenXRViewportCompositionLayerProvider::_update_swapchain_sub_image(XrSwapchainSubImage &r_subimage) {
	XrSwapchain swapchain = swapchain_info.get_swapchain();

	if (swapchain && swapchain_info.is_image_acquired()) {
		swapchain_info.release();
	}

	r_subimage.swapchain = swapchain;
	r_subimage.imageRect.extent.width = swapchain_size.width;
	r_subimage.imageRect.extent.height = swapchain_size.height;
}

OpenXRViewportCompositionLayerProvider::~OpenXRViewportCompositionLayerProvider() {
	// This will reset the viewport and free the swapchain too.
	set_viewport(RID(), Size2i());
}

bool OpenXRViewportCompositionLayerProvider::update_and_acquire_swapchain(bool p_static_image) {
	if (openxr_api == nullptr || composition_layer_extension == nullptr) {
		// OpenXR not initialized or we're in the editor?
		return false;
	}
	if (!composition_layer_extension->is_available(get_openxr_type())) {
		// Selected type is not supported?
		return false;
	}

	// See if our current swapchain is outdated.
	if (swapchain_info.get_swapchain() != XR_NULL_HANDLE) {
		// If this swap chain, or the previous one, were static, then we can't reuse it.
		if (swapchain_size == viewport_size && !p_static_image && !static_image) {
			// We're all good! Just acquire it.
			// We can ignore should_render here, return will be false.
			bool should_render = true;
			return swapchain_info.acquire(should_render);
		}

		swapchain_info.queue_free();
	}

	// Create our new swap chain
	int64_t swapchain_format = openxr_api->get_color_swapchain_format();
	const uint32_t sample_count = 1;
	const uint32_t array_size = 1;
	XrSwapchainCreateFlags create_flags = 0;
	if (p_static_image) {
		create_flags |= XR_SWAPCHAIN_CREATE_STATIC_IMAGE_BIT;
	}
	if (!swapchain_info.create(create_flags, XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_MUTABLE_FORMAT_BIT, swapchain_format, viewport_size.width, viewport_size.height, sample_count, array_size)) {
		swapchain_size = Size2i();
		return false;
	}

	// Acquire our image so we can start rendering into it,
	// we can ignore should_render here, ret will be false.
	bool should_render = true;
	bool ret = swapchain_info.acquire(should_render);

	swapchain_size = viewport_size;
	static_image = p_static_image;
	return ret;
}

void OpenXRViewportCompositionLayerProvider::free_swapchain() {
	if (swapchain_info.get_swapchain() != XR_NULL_HANDLE) {
		swapchain_info.queue_free();
	}

	swapchain_size = Size2i();
	static_image = false;
}

RID OpenXRViewportCompositionLayerProvider::get_current_swapchain_texture() {
	if (openxr_api == nullptr) {
		return RID();
	}

	return swapchain_info.get_image();
}

////////////////////////////////////////////////////////////////////////////
// OpenXRAndroidSurfaceCompositionLayerProvider

#ifdef ANDROID_ENABLED
bool OpenXRAndroidSurfaceCompositionLayerProvider::create_swapchain() {
	ERR_FAIL_COND_V(swapchain != XR_NULL_HANDLE, false);
	ERR_FAIL_COND_V(jobject != nullptr, false);
	ERR_FAIL_COND_V(surface_size == Size2i(), false);

	XrSwapchainCreateInfo swapchain_info = {
		XR_TYPE_SWAPCHAIN_CREATE_INFO, // type
		nullptr, // next
		0, // createFlags
		XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_MUTABLE_FORMAT_BIT, // usageFlags
		0, // format
		1, // sampleCount
		surface_size.x, // width
		surface_size.y, // height
		1, // faceCount
		1, // arraySize
		1, // mipCount
	};
	return composition_layer_extension->create_android_surface_swapchain(&swapchain_info, &swapchain, &surface);
}

void OpenXRAndroidSurfaceCompositionLayerProvider::free_swapchain() {
	if (swapchain != XR_NULL_HANDLE) {
		composition_layer_extension->free_android_surface_swapchain(swapchain);

		swapchain = XR_NULL_HANDLE;
		surface = nullptr;
	}
}
#endif

void OpenXRAndroidSurfaceCompositionLayerProvider::_update_swapchain_sub_image(XrSwapchainSubImage &r_subimage) {
#ifdef ANDROID_ENABLED
	if (swapchain == XR_NULL_HANDLE) {
		create_swapchain();
	}

	r_subimage.swapchain = swapchain;
	r_subimage.extents.width = surface_size.width;
	r_subimage.extents.height = surface_size.height;
#endif
}

void OpenXRAndroidSurfaceCompositionLayerProvider::set_surface_size(Size2i p_size) {
#ifdef ANDROID_ENABLED
	if (surface_size != p_size) {
		surface_size = p_size;
	}
#endif
}

OpenXRAndroidSurfaceCompositionLayerProvider::~OpenXRAndroidSurfaceCompositionLayerProvider() {
#ifdef ANDROID_ENABLED
	free_swapchain();
#endif
}
