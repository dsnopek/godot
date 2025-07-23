/**************************************************************************/
/*  xr_debugger.cpp                                                       */
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

#ifdef DEBUG_ENABLED

#include "xr_debugger.h"

#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/xr/xr_nodes.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/3d/primitive_meshes.h"

XRDebuggerRuntimeEditor *XRDebugger::get_runtime_editor() {
	return runtime_editor_id.is_valid() ? Object::cast_to<XRDebuggerRuntimeEditor>(ObjectDB::get_instance(runtime_editor_id)) : nullptr;
}

void XRDebugger::initialize() {
	SceneTree *scene_tree = SceneTree::get_singleton();
	ERR_FAIL_NULL(scene_tree);

	XRDebuggerRuntimeEditor *runtime_editor = get_runtime_editor();
	if (runtime_editor) {
		return;
	}

	runtime_editor = memnew(XRDebuggerRuntimeEditor);
	runtime_editor_id = runtime_editor->get_instance_id();

	scene_tree->get_root()->add_child(runtime_editor);
}

void XRDebugger::deinitialize() {
	SceneTree *scene_tree = SceneTree::get_singleton();
	ERR_FAIL_NULL(scene_tree);

	XRDebuggerRuntimeEditor *runtime_editor = get_runtime_editor();
	if (runtime_editor) {
		return;
	}

	runtime_editor_id = ObjectID();
	runtime_editor->queue_free();
	scene_tree->get_root()->remove_child(runtime_editor);
}

XRDebugger::XRDebugger() {
}

XRDebugger::~XRDebugger() {
	deinitialize();
}

void XRDebuggerRuntimeEditor::_on_controller_button_pressed(const String &p_name) {
	// @todo We need to have an action set for OpenXR, and it needs to know to switch to it - not sure how to handle that yet.
	if (p_name == "menu_button") {
		set_enabled(!enabled);
	}
}

void XRDebuggerRuntimeEditor::set_enabled(bool p_enable) {
	if (enabled == p_enable) {
		return;
	}
	enabled = p_enable;

	if (enabled) {
		print_line("Do the thing!");
		xr_controller_left->set_visible(true);
		xr_controller_right->set_visible(true);
	} else {
		print_line("Stop doing the thing");
		xr_controller_left->set_visible(false);
		xr_controller_right->set_visible(false);
	}
}

XRDebuggerRuntimeEditor::XRDebuggerRuntimeEditor() {
	set_name("XRDebuggerRuntimeEditor");
	set_current(false);
	set_process_mode(Node::PROCESS_MODE_ALWAYS);

	xr_camera = memnew(XRCamera3D);
	xr_camera->set_current(false);
	add_child(xr_camera);

	xr_controller_left = memnew(XRController3D);
	xr_controller_left->set_tracker("left_hand");
	xr_controller_left->set_visible(false);
	xr_controller_left->connect("button_pressed", callable_mp(this, &XRDebuggerRuntimeEditor::_on_controller_button_pressed));
	add_child(xr_controller_left);

	Ref<BoxMesh> box_mesh;
	box_mesh.instantiate();
	box_mesh->set_size(Vector3(0.4, 0.4, 0.05));

	MeshInstance3D *left_mesh_instance = memnew(MeshInstance3D);
	left_mesh_instance->set_mesh(box_mesh);
	xr_controller_left->add_child(left_mesh_instance);

	xr_controller_right = memnew(XRController3D);
	xr_controller_right->set_tracker("right_hand");
	xr_controller_right->set_visible(false);
	add_child(xr_controller_right);

	Ref<BoxMesh> ray_mesh;
	ray_mesh.instantiate();
	ray_mesh->set_size(Vector3(0.01, 0.01, 10.0));

	MeshInstance3D *right_mesh_instance = memnew(MeshInstance3D);
	right_mesh_instance->set_mesh(ray_mesh);
	right_mesh_instance->set_position(Vector3(0.0, 0.0, -5.0));
	xr_controller_right->add_child(right_mesh_instance);
}

XRDebuggerRuntimeEditor::~XRDebuggerRuntimeEditor() {
}

#endif
