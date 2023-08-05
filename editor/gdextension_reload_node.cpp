/**************************************************************************/
/*  gdextension_reload_node.cpp                                           */
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

#include "gdextension_reload_node.h"

#include "core/extension/gdextension_manager.h"
#include "editor/editor_inspector.h"
#include "editor/editor_interface.h"

void GDExtensionReloadNode::_bind_methods() {
}

void GDExtensionReloadNode::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_APPLICATION_FOCUS_OUT: {
			unfocused = true;
		} break;

		case NOTIFICATION_APPLICATION_FOCUS_IN: {
			callable_mp(this, &GDExtensionReloadNode::_deferred_reload).call_deferred();
		} break;
	}
}

void GDExtensionReloadNode::_deferred_reload() {
	if (!unfocused) {
		return;
	}
	unfocused = false;

	GDExtensionManager *gdextension_manager = GDExtensionManager::get_singleton();

	Vector<String> extensions = gdextension_manager->get_loaded_extensions();
	bool reloaded = false;
	for (const String &extension_name : extensions) {
		Ref<GDExtension> extension = gdextension_manager->get_extension(extension_name);

		if (!extension->is_reloadable()) {
			continue;
		}

		if (extension->has_library_changed()) {
			reloaded = true;
			gdextension_manager->reload_extension(extension->get_path());
		}
	}

	if (reloaded) {
		// In case the developer is inspecting a node whose class was reloaded.
		EditorInterface::get_singleton()->get_inspector()->update_tree();
	}
}