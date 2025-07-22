/**************************************************************************/
/*  xr_debugger.h                                                         */
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

#ifdef DEBUG_ENABLED

#include "core/object/object.h"
#include "core/templates/local_vector.h"

class SceneDebugger;
class XROrigin3D;
class XRCamera3D;
class XRController3D;

class XRDebugger : public Object {
	GDSOFTCLASS(XRDebugger, Object);

	friend class SceneDebugger;

	inline static XRDebugger *singleton = nullptr;

	bool initialized = false;
	bool enabled = false;

	ObjectID original_xr_origin;
	ObjectID original_xr_camera;
	LocalVector<ObjectID> original_xr_controllers;

	XROrigin3D *xr_origin = nullptr;
	XRCamera3D *xr_camera = nullptr;
	XRController3D *xr_controller_left = nullptr;
	XRController3D *xr_controller_right = nullptr;

	void _on_controller_button_pressed(const String &p_name);

	void set_enabled(bool p_enable);

public:
	static XRDebugger *get_singleton() { return singleton; }

	void initialize();
	void deinitialize();

	XRDebugger();
	~XRDebugger();
};

#endif
