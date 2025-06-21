/**************************************************************************/
/*  gdextension_interface_header_generator.cpp                            */
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

#include "gdextension_interface_header_generator.h"

#include "core/io/json.h"
#include "gdextension_interface_dump.gen.h"

static const char *FILE_HEADER =
		"/**************************************************************************/\n"
		"/*  gdextension_interface.h                                               */\n";

static const char *INTRO =
		"\n"
		"#pragma once\n"
		"\n"
		"/* This is a C class header, you can copy it and use it directly in your own binders.\n"
		" * Together with the `extension_api.json` file, you should be able to generate any binder.\n"
		" */\n"
		"\n"
		"#ifndef __cplusplus\n"
		"#include <stddef.h>\n"
		"#include <stdint.h>\n"
		"\n"
		"typedef uint32_t char32_t;\n"
		"typedef uint16_t char16_t;\n"
		"#else\n"
		"#include <cstddef>\n"
		"#include <cstdint>\n"
		"\n"
		"extern \" C \" {\n"
		"#endif\n"
		"\n";

static const char *OUTRO =
		"\n"
		"#ifdef __cplusplus\n"
		"}\n"
		"#endif\n";

void GDExtensionInterfaceHeaderGenerator::generate_gdextension_interface_header(const String &p_path) {
	Ref<FileAccess> fa = FileAccess::open(p_path, FileAccess::WRITE);
	ERR_FAIL_COND_MSG(fa.is_null(), vformat("Cannot open file '%s' for writing.", p_path));

	Vector<uint8_t> bytes = GDExtensionInterfaceDump::load_gdextension_interface_file();
	String json_string = String::utf8(Span<char>((char *)bytes.ptr(), bytes.size()));

	Ref<JSON> json;
	json.instantiate();
	Error err = json->parse(json_string);
	ERR_FAIL_COND(err);

	Dictionary data = json->get_data();
	ERR_FAIL_COND(data.size() == 0);

	fa->store_string(FILE_HEADER);

	Array copyright = data["_copyright"];
	for (const Variant &line : copyright) {
		fa->store_line(line);
	}

	fa->store_string(INTRO);

	Array types = data["types"];
	for (const Variant &type : types) {
		Dictionary type_dict = type;
		if (type_dict.has("doc")) {
			write_doc(fa, type_dict["doc"]);
		}
	}

	fa->store_string(OUTRO);
}

void GDExtensionInterfaceHeaderGenerator::write_doc(const Ref<FileAccess> &p_fa, const Array &p_doc, const String &p_indent) {
	if (p_doc.size() == 1) {
		p_fa->store_string(vformat("%s/* %s */\n", p_indent, p_doc[0]));
		return;
	}

	bool first = true;
	for (const Variant &line : p_doc) {
		if (first) {
			p_fa->store_string(p_indent + "/* ");
			first = false;
		} else {
			p_fa->store_string(p_indent + " * ");
		}

		p_fa->store_line(line);
	}

	p_fa->store_line(p_indent + " */\n");
}

void GDExtensionInterfaceHeaderGenerator::write_simple_type(const Ref<FileAccess> &p_fa, const Dictionary &p_type) {
}

void GDExtensionInterfaceHeaderGenerator::write_enum_type(const Ref<FileAccess> &p_fa, const Dictionary &p_enum) {
}

void GDExtensionInterfaceHeaderGenerator::write_function_type(const Ref<FileAccess> &p_fa, const Dictionary &p_func) {
}

void GDExtensionInterfaceHeaderGenerator::write_struct_type(const Ref<FileAccess> &p_fa, const Dictionary &p_struct) {
}

String GDExtensionInterfaceHeaderGenerator::make_args_text(const Array &p_args) {
	return "";
}

void GDExtensionInterfaceHeaderGenerator::write_interface(const Ref<FileAccess> &p_fa, const Dictionary &p_interface) {
}
