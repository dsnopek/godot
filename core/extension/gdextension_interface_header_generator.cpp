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
		String type_type = type_dict["type"];
		if (type_type == "simple") {
			write_simple_type(fa, type_dict);
		} else if (type_type == "enum") {
			write_enum_type(fa, type_dict);
		} else if (type_type == "function") {
			write_function_type(fa, type_dict);
		} else if (type_type == "struct") {
			write_struct_type(fa, type_dict);
		}
	}

	Array interfaces = data["interface"];
	for (const Variant &interface : interfaces) {
		write_interface(fa, interface);
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

	p_fa->store_string(p_indent + " */\n");
}

void GDExtensionInterfaceHeaderGenerator::write_simple_type(const Ref<FileAccess> &p_fa, const Dictionary &p_type) {
	String def = p_type["def"];
	p_fa->store_string("typedef " + def);
	if (!def.ends_with("*")) {
		p_fa->store_string(" ");
	}
	p_fa->store_string((String)p_type["name"] + ";\n");
}

void GDExtensionInterfaceHeaderGenerator::write_enum_type(const Ref<FileAccess> &p_fa, const Dictionary &p_enum) {
	p_fa->store_string("typedef enum {\n");
	Array members = p_enum["members"];
	for (const Variant &member : members) {
		Dictionary member_dict = member;
		if (member_dict.has("doc")) {
			write_doc(p_fa, member_dict["doc"], "\t");
		}
		p_fa->store_string(vformat("\t%s = %s,\n", member_dict["name"], (int)member_dict["value"]));
	}
	p_fa->store_string(vformat("} %s;\n\n", p_enum["name"]));
}

void GDExtensionInterfaceHeaderGenerator::write_function_type(const Ref<FileAccess> &p_fa, const Dictionary &p_func) {
	Dictionary ret = p_func["ret"];
	String ret_type = ret["type"];
	p_fa->store_string("typedef " + ret_type);
	if (!ret_type.ends_with("*")) {
		p_fa->store_string(" ");
	}
	String args_text = p_func.has("args") ? make_args_text(p_func["args"]) : "";
	p_fa->store_string(vformat("(*%s)(%s);\n", p_func["name"], args_text));
}

void GDExtensionInterfaceHeaderGenerator::write_struct_type(const Ref<FileAccess> &p_fa, const Dictionary &p_struct) {
	p_fa->store_string("typedef struct {\n");
	Array members = p_struct["members"];
	for (const Variant &member : members) {
		Dictionary member_dict = member;
		if (member_dict.has("doc")) {
			write_doc(p_fa, member_dict["doc"], "\t");
		}
		String member_type = member_dict["type"];
		p_fa->store_string("\t" + member_type);
		if (!member_type.ends_with("*")) {
			p_fa->store_string(" ");
		}
		p_fa->store_string((String)member_dict["name"] + ";\n");
	}
	p_fa->store_string(vformat("} %s;\n\n", p_struct["name"]));
}

String GDExtensionInterfaceHeaderGenerator::make_args_text(const Array &p_args) {
	Vector<String> combined;
	for (const Variant &arg : p_args) {
		Dictionary arg_dict = arg;
		String arg_text = arg_dict["type"];
		if (arg_dict.has("name")) {
			if (!arg_text.ends_with("*")) {
				arg_text += " ";
			}
			arg_text += (String)arg_dict["name"];
		}
		combined.push_back(arg_text);
	}
	return String(", ").join(combined);
}

void GDExtensionInterfaceHeaderGenerator::write_interface(const Ref<FileAccess> &p_fa, const Dictionary &p_interface) {
}
