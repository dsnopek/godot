/**************************************************************************/
/*  openxr_composition_layer_extension.h                                  */
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

#ifndef OPENXR_COMPOSITION_LAYER_EXTENSION_H
#define OPENXR_COMPOSITION_LAYER_EXTENSION_H

#include "openxr_composition_layer_provider.h"
#include "openxr_extension_wrapper.h"

#include "../openxr_api.h"

#ifdef ANDROID_ENABLED
#include <jni.h>
#endif

class OpenXRViewportCompositionLayerProviderBase;

// This extension provides access to composition layers for displaying 2D content through the XR compositor.

// OpenXRCompositionLayerExtension enables the extensions related to this functionality
class OpenXRCompositionLayerExtension : public OpenXRExtensionWrapper, public OpenXRCompositionLayerProvider {
public:
	static OpenXRCompositionLayerExtension *get_singleton();

	OpenXRCompositionLayerExtension();
	virtual ~OpenXRCompositionLayerExtension() override;

	virtual HashMap<String, bool *> get_requested_extensions() override;
	virtual void on_instance_created(const XrInstance p_instance) override;
	virtual void on_session_created(const XrSession p_session) override;
	virtual void on_session_destroyed() override;
	virtual void on_pre_render() override;

	virtual int get_composition_layer_count() override;
	virtual XrCompositionLayerBaseHeader *get_composition_layer(int p_index) override;
	virtual int get_composition_layer_order(int p_index) override;

	void register_viewport_composition_layer_provider(OpenXRViewportCompositionLayerProviderBase *p_composition_layer);
	void unregister_viewport_composition_layer_provider(OpenXRViewportCompositionLayerProviderBase *p_composition_layer);

	bool is_available(XrStructureType p_which);

#ifdef ANDROID_ENABLED
	bool create_android_surface_swapchain(XrSwapchainCreateInfo *p_info, XrSwapchain *r_swapchain, jobject *r_surface);
	void free_android_surface_swapchain(XrSwapchain p_swapchain);
#endif

private:
	static OpenXRCompositionLayerExtension *singleton;

	Vector<OpenXRViewportCompositionLayerProviderBase *> composition_layers;

	bool cylinder_ext_available = false;
	bool equirect_ext_available = false;
	bool android_surface_ext_available = false;

#ifdef ANDROID_ENABLED
	Vector<XrSwapchain> android_surface_swapchain_free_queue;
	void free_queued_android_surface_swapchains();

	EXT_PROTO_XRRESULT_FUNC1(xrDestroySwapchain, (XrSwapchain), swapchain)
	EXT_PROTO_XRRESULT_FUNC4(xrCreateSwapchainAndroidSurfaceKHR, (XrSession), session, (const XrSwapchainCreateInfo *), info, (xrSwapchain *), swapchain, (jobject *), surface)
#endif
};

class OpenXRViewportCompositionLayerProviderBase {
	XrCompositionLayerBaseHeader *composition_layer = nullptr;
	int sort_order = 1;
	bool alpha_blend = false;
	Dictionary extension_property_values;
	bool extension_property_values_changed = true;

protected:
	OpenXRAPI *openxr_api = nullptr;
	OpenXRCompositionLayerExtension *composition_layer_extension = nullptr;

	// Called each frame, right before putting the swapchain into the composition layer settings.
	virtual void _update_swapchain_sub_image(XrSwapchainSubImage &r_swapchain_sub_image) = 0;

public:
	XrStructureType get_openxr_type() { return composition_layer->type; }

	void set_sort_order(int p_sort_order) { sort_order = p_sort_order; }
	int get_sort_order() const { return sort_order; }

	void set_alpha_blend(bool p_alpha_blend);
	bool get_alpha_blend() const { return alpha_blend; }

	void set_extension_property_values(const Dictionary &p_property_values);

	virtual void on_pre_render(){};
	XrCompositionLayerBaseHeader *get_composition_layer();

	OpenXRViewportCompositionLayerProviderBase(XrCompositionLayerBaseHeader *p_composition_layer);
	virtual ~OpenXRViewportCompositionLayerProviderBase();
};

class OpenXRViewportCompositionLayerProvider : public OpenXRViewportCompositionLayerProviderBase {
	RID viewport;
	Size2i viewport_size;

	OpenXRAPI::OpenXRSwapChainInfo swapchain_info;
	Size2i swapchain_size;
	bool static_image = false;

	bool update_and_acquire_swapchain(bool p_static_image);
	void free_swapchain();
	RID get_current_swapchain_texture();

protected:
	virtual void _update_swapchain_sub_image(XrSwapchainSubImage &r_swapchain_sub_image) override;

public:
	void set_viewport(RID p_viewport, Size2i p_size);
	RID get_viewport() const { return viewport; }

	virtual void on_pre_render() override;

	virtual ~OpenXRViewportCompositionLayerProvider();
};

class OpenXRAndroidSurfaceCompositionLayerProvider : public OpenXRViewportCompositionLayerProviderBase {
#ifdef ANDROID_ENABLED
	jobject surface;
	XrSwapchain swapchain = XR_NULL_HANDLE;
	Size2i surface_size;

	bool create_swapchain();
	void free_swapchain();
#endif

protected:
	virtual void _update_swapchain_sub_image(XrSwapchainSubImage &r_swapchain_sub_image) override;

public:
	// @todo me!
	//Ref<JavaObject> get_android_surface();

	void set_surface_size(Size2i p_size);

	virtual ~OpenXRAndroidSurfaceCompositionLayerProvider();
};

#endif // OPENXR_COMPOSITION_LAYER_EXTENSION_H
