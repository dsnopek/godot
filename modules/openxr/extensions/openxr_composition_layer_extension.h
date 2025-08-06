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

#pragma once

#include "openxr_extension_wrapper.h"

#include "../openxr_api.h"

#ifdef ANDROID_ENABLED
#include <jni.h>

// Copied here from openxr_platform.h, in order to avoid including that whole header,
// which can cause compilation issues on some platforms.
typedef XrResult(XRAPI_PTR *PFN_xrCreateSwapchainAndroidSurfaceKHR)(XrSession session, const XrSwapchainCreateInfo *info, XrSwapchain *swapchain, jobject *surface);
#endif

class JavaObject;

// This extension provides access to composition layers for displaying 2D content through the XR compositor.

#define OPENXR_LAYER_FUNC1(m_name, m_arg1)                                                                                                                       \
	void _composition_layer_##m_name##_rt(RID p_layer, m_arg1 p1) {                                                                                              \
		CompositionLayer *layer = composition_layer_owner.get_or_null(p_layer);                                                                                  \
		ERR_FAIL_NULL(layer);                                                                                                                                    \
		layer->m_name(p1);                                                                                                                                       \
	}                                                                                                                                                            \
	void composition_layer_##m_name(RID p_layer, m_arg1 p1) {                                                                                                    \
		RenderingServer::get_singleton()->call_on_render_thread(callable_mp(this, &OpenXRCompositionLayerExtension::_composition_layer_##m_name##_rt).bind(p1)); \
	}

#define OPENXR_LAYER_FUNC2(m_name, m_arg1, m_arg2)                                                                                                                   \
	void _composition_layer_##m_name##_rt(RID p_layer, m_arg1 p1, m_arg2 p2) {                                                                                       \
		CompositionLayer *layer = composition_layer_owner.get_or_null(p_layer);                                                                                      \
		ERR_FAIL_NULL(layer);                                                                                                                                        \
		layer->m_name(p1, p2);                                                                                                                                       \
	}                                                                                                                                                                \
	void composition_layer_##m_name(RID p_layer, m_arg1 p1, m_arg2 p2) {                                                                                             \
		RenderingServer::get_singleton()->call_on_render_thread(callable_mp(this, &OpenXRCompositionLayerExtension::_composition_layer_##m_name##_rt).bind(p1, p2)); \
	}

// @todo There was a comment here before - bring it back!
class OpenXRCompositionLayerExtension : public OpenXRExtensionWrapper {
	GDCLASS(OpenXRCompositionLayerExtension, OpenXRExtensionWrapper);

protected:
	static void _bind_methods() {}

public:
	/*
	struct SwapchainState {
		OpenXRCompositionLayer::Filter min_filter = OpenXRCompositionLayer::FILTER_LINEAR;
		OpenXRCompositionLayer::Filter mag_filter = OpenXRCompositionLayer::FILTER_LINEAR;
		OpenXRCompositionLayer::MipmapMode mipmap_mode = OpenXRCompositionLayer::MIPMAP_MODE_LINEAR;
		OpenXRCompositionLayer::Wrap horizontal_wrap = OpenXRCompositionLayer::WRAP_CLAMP_TO_BORDER;
		OpenXRCompositionLayer::Wrap vertical_wrap = OpenXRCompositionLayer::WRAP_CLAMP_TO_BORDER;
		OpenXRCompositionLayer::Swizzle red_swizzle = OpenXRCompositionLayer::SWIZZLE_RED;
		OpenXRCompositionLayer::Swizzle green_swizzle = OpenXRCompositionLayer::SWIZZLE_GREEN;
		OpenXRCompositionLayer::Swizzle blue_swizzle = OpenXRCompositionLayer::SWIZZLE_BLUE;
		OpenXRCompositionLayer::Swizzle alpha_swizzle = OpenXRCompositionLayer::SWIZZLE_ALPHA;
		float max_anisotropy = 1.0;
		Color border_color = { 0.0, 0.0, 0.0, 0.0 };
	};
	*/

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

	// The data on p_openxr_layer will be copied - there is no need to keep it valid after this call.
	RID composition_layer_create(XrCompositionLayerBaseHeader *p_openxr_layer);
	void composition_layer_free(RID p_layer);

	void composition_layer_register(RID p_layer);
	void composition_layer_unregister(RID p_layer);

	// @todo all the forwarding methods
	OPENXR_LAYER_FUNC2(set_viewport, RID, const Size2i &);
	OPENXR_LAYER_FUNC2(set_use_android_surface, bool, const Size2i &);
	OPENXR_LAYER_FUNC1(set_sort_order, int);
	OPENXR_LAYER_FUNC1(set_alpha_blend, bool);
	OPENXR_LAYER_FUNC1(set_transform, const Transform3D &);
	// @todo All the swapchain state stuff
	OPENXR_LAYER_FUNC1(set_extension_property_values, Dictionary)

	OPENXR_LAYER_FUNC1(set_quad_size, const Size2 &);

	bool is_available(XrStructureType p_which);
	bool is_android_surface_swapchain_available() { return android_surface_ext_available; }

private:
	static OpenXRCompositionLayerExtension *singleton;

	bool cylinder_ext_available = false;
	bool equirect_ext_available = false;
	bool android_surface_ext_available = false;

	void _composition_layer_free_rt(RID p_layer);
	void _composition_layer_register_rt(RID p_layer);
	void _composition_layer_unregister_rt(RID p_layer);

#ifdef ANDROID_ENABLED
	bool create_android_surface_swapchain(XrSwapchainCreateInfo *p_info, XrSwapchain *r_swapchain, jobject *r_surface);
	void free_android_surface_swapchain(XrSwapchain p_swapchain);

	EXT_PROTO_XRRESULT_FUNC1(xrDestroySwapchain, (XrSwapchain), swapchain)
	EXT_PROTO_XRRESULT_FUNC4(xrCreateSwapchainAndroidSurfaceKHR, (XrSession), session, (const XrSwapchainCreateInfo *), info, (XrSwapchain *), swapchain, (jobject *), surface)
#endif

	struct CompositionLayer {
		union {
			XrCompositionLayerBaseHeader composition_layer;
			XrCompositionLayerQuad composition_layer_quad;
			XrCompositionLayerCylinderKHR composition_layer_cylinder;
			XrCompositionLayerEquirect2KHR composition_layer_equirect;
		};

		int sort_order = 1;
		bool alpha_blend = false;
		Dictionary extension_property_values;
		bool extension_property_values_changed = true;

		struct {
			RID viewport;
			Size2i viewport_size;
			OpenXRAPI::OpenXRSwapChainInfo swapchain_info;
			bool static_image = false;
		} subviewport;

#ifdef ANDROID_ENABLED
		struct {
			XrSwapchain swapchain = XR_NULL_HANDLE;
			Ref<JavaObject> surface;
			Mutex mutex;
		} android_surface;
#endif

		bool use_android_surface = false;
		Size2i swapchain_size;

		//SwapchainState swapchain_state;
		bool swapchain_state_is_dirty = false;

		void set_viewport(RID p_viewport, const Size2i &p_size);
		void set_use_android_surface(bool p_use_android_surface, const Size2i &p_size);

		void set_sort_order(int p_sort_order) { sort_order = p_sort_order; }
		void set_alpha_blend(bool p_alpha_blend);
		void set_transform(const Transform3D &p_transform);
		void set_extension_property_values(const Dictionary &p_extension_property_values);

		void set_quad_size(const Size2 &p_size);

		Ref<JavaObject> get_android_surface();
		void on_pre_render();
		XrCompositionLayerBaseHeader *get_composition_layer();
		void free();

	private:
		void update_swapchain_state();
		void update_swapchain_sub_image(XrSwapchainSubImage &r_subimage);
		bool update_and_acquire_swapchain(bool p_static_image);
		RID get_current_swapchain_texture();
		void free_swapchain();

#ifdef ANDROID_ENABLED
		void create_android_surface();
#endif
	};

	RID_Owner<CompositionLayer, true> composition_layer_owner;
	Vector<CompositionLayer *> registered_composition_layers;
};

#undef OPENXR_LAYER_FUNC1
#undef OPENXR_LAYER_FUNC2
