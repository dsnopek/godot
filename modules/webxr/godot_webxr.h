/*************************************************************************/
/*  godot_webxr.h                                                        */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2020 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2020 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#ifndef GODOT_WEBXR_H
#define GODOT_WEBXR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stddef.h"

extern int godot_webxr_is_supported();
extern void godot_webxr_is_session_supported(const char *p_session_mode);

extern void godot_webxr_initialize(const char *p_session_mode, const char *p_required_features, const char *p_optional_features, const char *p_requested_reference_space_types);
extern void godot_webxr_uninitialize();

extern int *godot_webxr_get_render_targetsize();
extern float *godot_webxr_get_transform_for_eye(int p_eye);
extern float *godot_webxr_get_projection_for_eye(int p_eye);
extern int godot_webxr_get_external_texture_for_eye(int p_eye);
extern void godot_webxr_commit_for_eye(int p_eye);

extern void godot_webxr_sample_input_sources();
extern int godot_webxr_get_input_source_count();
extern int godot_webxr_is_input_source_connected(int p_input_source);
extern int godot_webxr_get_input_source_target_ray_mode(int p_input_source);
extern float *godot_webxr_get_input_source_matrix(int p_input_source, int p_input_pose);
extern int godot_webxr_get_controller_count();
extern int *godot_webxr_get_controller_buttons(int p_controller);
extern int *godot_webxr_get_controller_axes(int p_controller);

#ifdef __cplusplus
}
#endif

#endif /* GODOT_WEBXR_H */
