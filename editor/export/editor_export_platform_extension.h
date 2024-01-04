/**************************************************************************/
/*  editor_export_platform_extension.h                                    */
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

#ifndef EDITOR_EXPORT_PLATFORM_EXTENSION_H
#define EDITOR_EXPORT_PLATFORM_EXTENSION_H

#include "core/extension/ext_wrappers.gen.inc"
#include "editor_export_platform.h"

class EditorExportPlatformExtension : public EditorExportPlatform {
	GDCLASS(EditorExportPlatformExtension, EditorExportPlatform);

protected:
	static void _bind_methods();

public:
	GDVIRTUAL1RC(TypedArray<String>, _get_preset_features, const Ref<EditorExportPreset> &);

	void get_preset_features(const Ref<EditorExportPreset> &p_preset, List<String> *r_features) const override {
		TypedArray<String> ret;
		if (GDVIRTUAL_CALL(_get_preset_features, p_preset, ret)) {
			for (int i = 0; i < ret.size(); i++) {
				r_features->push_back(ret[i]);
			}
		}
	}

	GDVIRTUAL1RC(bool, _is_executable, const String &);

	bool is_executable(const String &p_path) const override {
		bool ret = false;
		GDVIRTUAL_CALL(_is_executable, p_path, ret);
		return ret;
	}

	GDVIRTUAL0RC(TypedArray<Dictionary>, _get_export_options);

	void get_export_options(List<ExportOption> *r_options) const override {
		TypedArray<Dictionary> ret;
		if (GDVIRTUAL_CALL(_get_export_options, ret)) {
			for (int i = 0; i < ret.size(); i++) {
				EditorExportPlatform::ExportOption option;
				if (EditorExportPlatform::ExportOption::from_dict(ret[i], option)) {
					r_options->push_back(option);
				}
			}
		}
	}

	GDVIRTUAL0R(bool, _should_update_export_options);

	bool should_update_export_options() override {
		bool ret = false;
		GDVIRTUAL_CALL(_should_update_export_options, ret);
		return ret;
	}

	// @todo get_export_option_visibility
	// @todo get_export_option_warning

	EXBIND0RC(String, get_os_name);
	EXBIND0RC(String, get_name);
	EXBIND0RC(Ref<Texture2D>, get_logo);
};

#endif // EDITOR_EXPORT_PLATFORM_H
