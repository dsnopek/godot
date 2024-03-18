/**************************************************************************/
/*  openxr_composition_layer.cpp                                          */
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

#include "openxr_composition_layer.h"

#include "../extensions/openxr_composition_layer_extension.h"
#include "../openxr_api.h"
#include "../openxr_interface.h"

#include "scene/main/viewport.h"

OpenXRCompositionLayer::OpenXRCompositionLayer() {
	openxr_api = OpenXRAPI::get_singleton();

	Ref<OpenXRInterface> openxr_interface = XRServer::get_singleton()->find_interface("OpenXR");
	if (openxr_interface.is_valid()) {
		openxr_interface->connect("session_begun", callable_mp(this, &OpenXRCompositionLayer::_on_openxr_session_begun));
	}

	set_process_internal(true);
}

OpenXRCompositionLayer::~OpenXRCompositionLayer() {
	Ref<OpenXRInterface> openxr_interface = XRServer::get_singleton()->find_interface("OpenXR");
	if (openxr_interface.is_valid()) {
		openxr_interface->disconnect("session_begun", callable_mp(this, &OpenXRCompositionLayer::_on_openxr_session_begun));
	}

	if (openxr_layer_provider != nullptr) {
		memdelete(openxr_layer_provider);
		openxr_layer_provider = nullptr;
	}
}

void OpenXRCompositionLayer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_layer_viewport", "viewport"), &OpenXRCompositionLayer::set_layer_viewport);
	ClassDB::bind_method(D_METHOD("get_layer_viewport"), &OpenXRCompositionLayer::get_layer_viewport);

	ClassDB::bind_method(D_METHOD("set_sort_order", "order"), &OpenXRCompositionLayer::set_sort_order);
	ClassDB::bind_method(D_METHOD("get_sort_order"), &OpenXRCompositionLayer::get_sort_order);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "layer_viewport", PROPERTY_HINT_NODE_TYPE, "SubViewport"), "set_layer_viewport", "get_layer_viewport");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "sort_order", PROPERTY_HINT_NONE, ""), "set_sort_order", "get_sort_order");
}

void OpenXRCompositionLayer::set_layer_viewport(SubViewport *p_viewport) {
	layer_viewport = p_viewport;
	if (is_visible() && is_inside_tree() && openxr_layer_provider->set_viewport(p_viewport)) {
		if (layer_viewport) {
			_on_layer_viewport_changed();
		}
	}
}

SubViewport *OpenXRCompositionLayer::get_layer_viewport() const {
	return layer_viewport;
}

void OpenXRCompositionLayer::set_sort_order(int p_order) {
	if (openxr_layer_provider) {
		openxr_layer_provider->set_sort_order(p_order);
	}
}

int OpenXRCompositionLayer::get_sort_order() const {
	if (openxr_layer_provider) {
		return openxr_layer_provider->get_sort_order();
	}
	return 1;
}

void OpenXRCompositionLayer::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			if (layer_viewport) {
				_on_layer_viewport_changed();
			}
		} break;
		case NOTIFICATION_INTERNAL_PROCESS: {
			if (is_visible()) {
				openxr_layer_provider->process();
			}
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
