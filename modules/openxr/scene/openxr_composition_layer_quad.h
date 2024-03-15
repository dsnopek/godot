/**************************************************************************/
/*  openxr_composition_layer_quad.h                                       */
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

#ifndef OPENXR_COMPOSITION_LAYER_QUAD_H
#define OPENXR_COMPOSITION_LAYER_QUAD_H

#include <openxr/openxr.h>

#include "scene/3d/node_3d.h"

class MeshInstance3D;
class OpenXRAPI;
class OpenXRViewportCompositionLayerProvider;
class SubViewport;

class OpenXRCompositionLayerQuad : public Node3D {
	GDCLASS(OpenXRCompositionLayerQuad, Node3D);

	XrCompositionLayerQuad composition_layer;

	Size2 quad_size = Size2(1.0, 1.0);
	SubViewport *layer_viewport = nullptr;
	MeshInstance3D *fallback = nullptr;

	OpenXRAPI *openxr_api = nullptr;
	OpenXRViewportCompositionLayerProvider *openxr_layer_provider = nullptr;

	void _reset_fallback_material();

protected:
	static void _bind_methods();

	void _notification(int p_what);

public:
	void set_quad_size(const Size2 &p_size);
	Size2 get_quad_size() const;

	void set_layer_viewport(SubViewport *p_viewport);
	SubViewport *get_layer_viewport() const;

	OpenXRCompositionLayerQuad();
	~OpenXRCompositionLayerQuad();
};

#endif // OPENXR_COMPOSITION_LAYER_H
