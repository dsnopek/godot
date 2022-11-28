/*************************************************************************/
/*  dedicated_server_export_plugin.cpp                                   */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
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

#include "dedicated_server_export_plugin.h"

#define PRESET_DEDICATED_SERVER_PROPERTY "dedicated_server/is_server"

bool DedicatedServerExportPlugin::is_dedicated_server() const {
	Ref<EditorExportPreset> preset = get_export_preset();
	ERR_FAIL_COND_V(preset.is_null(), false);

	bool valid_prop = false;
	bool is_server = preset->get(PRESET_DEDICATED_SERVER_PROPERTY, &valid_prop);

	if (valid_prop) {
		return is_server;
	}

	return false;
}

PackedStringArray DedicatedServerExportPlugin::_get_export_features(const Ref<EditorExportPlatform> &p_platform, bool p_debug) const {
	PackedStringArray ret;
	if (is_dedicated_server()) {
		ret.append("dedicated_server");
	}
	return ret;
}

void DedicatedServerExportPlugin::add_export_option(List<EditorExportPlatform::ExportOption> *r_options) {
	r_options->push_back(EditorExportPlatform::ExportOption(PropertyInfo(Variant::BOOL, PRESET_DEDICATED_SERVER_PROPERTY), false));
}
