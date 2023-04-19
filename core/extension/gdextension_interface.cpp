/**************************************************************************/
/*  gdextension_interface.cpp                                             */
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

#include "gdextension_interface.h"

#include "core/config/engine.h"
#include "core/extension/gdextension.h"
#include "core/io/file_access.h"
#include "core/io/xml_parser.h"
#include "core/object/class_db.h"
#include "core/object/script_language_extension.h"
#include "core/object/worker_thread_pool.h"
#include "core/os/memory.h"
#include "core/variant/variant.h"
#include "core/version.h"

// Core interface functions.
static void *gdextension_get_proc_address(const char *p_name) {
	return GDExtension::get_interface_function(p_name);
}

static void gdextension_get_godot_version(GDExtensionGodotVersion *r_godot_version) {
	r_godot_version->major = VERSION_MAJOR;
	r_godot_version->minor = VERSION_MINOR;
	r_godot_version->patch = VERSION_PATCH;
	r_godot_version->string = VERSION_FULL_NAME;
}

// Memory Functions
static void *gdextension_mem_alloc(size_t p_size) {
	return memalloc(p_size);
}

static void *gdextension_mem_realloc(void *p_mem, size_t p_size) {
	return memrealloc(p_mem, p_size);
}

static void gdextension_mem_free(void *p_mem) {
	memfree(p_mem);
}

// Helper print functions.
static void gdextension_print_error(const char *p_description, const char *p_function, const char *p_file, int32_t p_line, GDExtensionBool p_editor_notify) {
	_err_print_error(p_function, p_file, p_line, p_description, p_editor_notify, ERR_HANDLER_ERROR);
}
static void gdextension_print_error_with_message(const char *p_description, const char *p_message, const char *p_function, const char *p_file, int32_t p_line, GDExtensionBool p_editor_notify) {
	_err_print_error(p_function, p_file, p_line, p_description, p_message, p_editor_notify, ERR_HANDLER_ERROR);
}
static void gdextension_print_warning(const char *p_description, const char *p_function, const char *p_file, int32_t p_line, GDExtensionBool p_editor_notify) {
	_err_print_error(p_function, p_file, p_line, p_description, p_editor_notify, ERR_HANDLER_WARNING);
}
static void gdextension_print_warning_with_message(const char *p_description, const char *p_message, const char *p_function, const char *p_file, int32_t p_line, GDExtensionBool p_editor_notify) {
	_err_print_error(p_function, p_file, p_line, p_description, p_message, p_editor_notify, ERR_HANDLER_WARNING);
}
static void gdextension_print_script_error(const char *p_description, const char *p_function, const char *p_file, int32_t p_line, GDExtensionBool p_editor_notify) {
	_err_print_error(p_function, p_file, p_line, p_description, p_editor_notify, ERR_HANDLER_SCRIPT);
}
static void gdextension_print_script_error_with_message(const char *p_description, const char *p_message, const char *p_function, const char *p_file, int32_t p_line, GDExtensionBool p_editor_notify) {
	_err_print_error(p_function, p_file, p_line, p_description, p_message, p_editor_notify, ERR_HANDLER_SCRIPT);
}

uint64_t gdextension_get_native_struct_size(GDExtensionConstStringNamePtr p_name) {
	const StringName name = *reinterpret_cast<const StringName *>(p_name);
	return ClassDB::get_native_struct_size(name);
}

// Variant functions

static void gdextension_variant_new_copy(GDExtensionVariantPtr r_dest, GDExtensionConstVariantPtr p_src) {
	memnew_placement(reinterpret_cast<Variant *>(r_dest), Variant(*reinterpret_cast<const Variant *>(p_src)));
}
static void gdextension_variant_new_nil(GDExtensionVariantPtr r_dest) {
	memnew_placement(reinterpret_cast<Variant *>(r_dest), Variant);
}
static void gdextension_variant_destroy(GDExtensionVariantPtr p_self) {
	reinterpret_cast<Variant *>(p_self)->~Variant();
}

// variant type

static void gdextension_variant_call(GDExtensionVariantPtr p_self, GDExtensionConstStringNamePtr p_method, const GDExtensionConstVariantPtr *p_args, GDExtensionInt p_argcount, GDExtensionVariantPtr r_return, GDExtensionCallError *r_error) {
	Variant *self = (Variant *)p_self;
	const StringName method = *reinterpret_cast<const StringName *>(p_method);
	const Variant **args = (const Variant **)p_args;
	Variant ret;
	Callable::CallError error;
	self->callp(method, args, p_argcount, ret, error);
	memnew_placement(r_return, Variant(ret));

	if (r_error) {
		r_error->error = (GDExtensionCallErrorType)(error.error);
		r_error->argument = error.argument;
		r_error->expected = error.expected;
	}
}

static void gdextension_variant_call_static(GDExtensionVariantType p_type, GDExtensionConstStringNamePtr p_method, const GDExtensionConstVariantPtr *p_args, GDExtensionInt p_argcount, GDExtensionVariantPtr r_return, GDExtensionCallError *r_error) {
	Variant::Type type = (Variant::Type)p_type;
	const StringName method = *reinterpret_cast<const StringName *>(p_method);
	const Variant **args = (const Variant **)p_args;
	Variant ret;
	Callable::CallError error;
	Variant::call_static(type, method, args, p_argcount, ret, error);
	memnew_placement(r_return, Variant(ret));

	if (r_error) {
		r_error->error = (GDExtensionCallErrorType)error.error;
		r_error->argument = error.argument;
		r_error->expected = error.expected;
	}
}

static void gdextension_variant_evaluate(GDExtensionVariantOperator p_op, GDExtensionConstVariantPtr p_a, GDExtensionConstVariantPtr p_b, GDExtensionVariantPtr r_return, GDExtensionBool *r_valid) {
	Variant::Operator op = (Variant::Operator)p_op;
	const Variant *a = (const Variant *)p_a;
	const Variant *b = (const Variant *)p_b;
	Variant *ret = (Variant *)r_return;
	bool valid;
	Variant::evaluate(op, *a, *b, *ret, valid);
	*r_valid = valid;
}

static void gdextension_variant_set(GDExtensionVariantPtr p_self, GDExtensionConstVariantPtr p_key, GDExtensionConstVariantPtr p_value, GDExtensionBool *r_valid) {
	Variant *self = (Variant *)p_self;
	const Variant *key = (const Variant *)p_key;
	const Variant *value = (const Variant *)p_value;

	bool valid;
	self->set(*key, *value, &valid);
	*r_valid = valid;
}

static void gdextension_variant_set_named(GDExtensionVariantPtr p_self, GDExtensionConstStringNamePtr p_key, GDExtensionConstVariantPtr p_value, GDExtensionBool *r_valid) {
	Variant *self = (Variant *)p_self;
	const StringName *key = (const StringName *)p_key;
	const Variant *value = (const Variant *)p_value;

	bool valid;
	self->set_named(*key, *value, valid);
	*r_valid = valid;
}

static void gdextension_variant_set_keyed(GDExtensionVariantPtr p_self, GDExtensionConstVariantPtr p_key, GDExtensionConstVariantPtr p_value, GDExtensionBool *r_valid) {
	Variant *self = (Variant *)p_self;
	const Variant *key = (const Variant *)p_key;
	const Variant *value = (const Variant *)p_value;

	bool valid;
	self->set_keyed(*key, *value, valid);
	*r_valid = valid;
}

static void gdextension_variant_set_indexed(GDExtensionVariantPtr p_self, GDExtensionInt p_index, GDExtensionConstVariantPtr p_value, GDExtensionBool *r_valid, GDExtensionBool *r_oob) {
	Variant *self = (Variant *)p_self;
	const Variant *value = (const Variant *)p_value;

	bool valid;
	bool oob;
	self->set_indexed(p_index, *value, valid, oob);
	*r_valid = valid;
	*r_oob = oob;
}

static void gdextension_variant_get(GDExtensionConstVariantPtr p_self, GDExtensionConstVariantPtr p_key, GDExtensionVariantPtr r_ret, GDExtensionBool *r_valid) {
	const Variant *self = (const Variant *)p_self;
	const Variant *key = (const Variant *)p_key;

	bool valid;
	memnew_placement(r_ret, Variant(self->get(*key, &valid)));
	*r_valid = valid;
}

static void gdextension_variant_get_named(GDExtensionConstVariantPtr p_self, GDExtensionConstStringNamePtr p_key, GDExtensionVariantPtr r_ret, GDExtensionBool *r_valid) {
	const Variant *self = (const Variant *)p_self;
	const StringName *key = (const StringName *)p_key;

	bool valid;
	memnew_placement(r_ret, Variant(self->get_named(*key, valid)));
	*r_valid = valid;
}

static void gdextension_variant_get_keyed(GDExtensionConstVariantPtr p_self, GDExtensionConstVariantPtr p_key, GDExtensionVariantPtr r_ret, GDExtensionBool *r_valid) {
	const Variant *self = (const Variant *)p_self;
	const Variant *key = (const Variant *)p_key;

	bool valid;
	memnew_placement(r_ret, Variant(self->get_keyed(*key, valid)));
	*r_valid = valid;
}

static void gdextension_variant_get_indexed(GDExtensionConstVariantPtr p_self, GDExtensionInt p_index, GDExtensionVariantPtr r_ret, GDExtensionBool *r_valid, GDExtensionBool *r_oob) {
	const Variant *self = (const Variant *)p_self;

	bool valid;
	bool oob;
	memnew_placement(r_ret, Variant(self->get_indexed(p_index, valid, oob)));
	*r_valid = valid;
	*r_oob = oob;
}

/// Iteration.
static GDExtensionBool gdextension_variant_iter_init(GDExtensionConstVariantPtr p_self, GDExtensionVariantPtr r_iter, GDExtensionBool *r_valid) {
	const Variant *self = (const Variant *)p_self;
	Variant *iter = (Variant *)r_iter;

	bool valid;
	bool ret = self->iter_init(*iter, valid);
	*r_valid = valid;
	return ret;
}

static GDExtensionBool gdextension_variant_iter_next(GDExtensionConstVariantPtr p_self, GDExtensionVariantPtr r_iter, GDExtensionBool *r_valid) {
	const Variant *self = (const Variant *)p_self;
	Variant *iter = (Variant *)r_iter;

	bool valid;
	bool ret = self->iter_next(*iter, valid);
	*r_valid = valid;
	return ret;
}

static void gdextension_variant_iter_get(GDExtensionConstVariantPtr p_self, GDExtensionVariantPtr r_iter, GDExtensionVariantPtr r_ret, GDExtensionBool *r_valid) {
	const Variant *self = (const Variant *)p_self;
	Variant *iter = (Variant *)r_iter;

	bool valid;
	memnew_placement(r_ret, Variant(self->iter_next(*iter, valid)));
	*r_valid = valid;
}

/// Variant functions.
static GDExtensionInt gdextension_variant_hash(GDExtensionConstVariantPtr p_self) {
	const Variant *self = (const Variant *)p_self;
	return self->hash();
}

static GDExtensionInt gdextension_variant_recursive_hash(GDExtensionConstVariantPtr p_self, GDExtensionInt p_recursion_count) {
	const Variant *self = (const Variant *)p_self;
	return self->recursive_hash(p_recursion_count);
}

static GDExtensionBool gdextension_variant_hash_compare(GDExtensionConstVariantPtr p_self, GDExtensionConstVariantPtr p_other) {
	const Variant *self = (const Variant *)p_self;
	const Variant *other = (const Variant *)p_other;
	return self->hash_compare(*other);
}

static GDExtensionBool gdextension_variant_booleanize(GDExtensionConstVariantPtr p_self) {
	const Variant *self = (const Variant *)p_self;
	return self->booleanize();
}

static void gdextension_variant_duplicate(GDExtensionConstVariantPtr p_self, GDExtensionVariantPtr r_ret, GDExtensionBool p_deep) {
	const Variant *self = (const Variant *)p_self;
	memnew_placement(r_ret, Variant(self->duplicate(p_deep)));
}

static void gdextension_variant_stringify(GDExtensionConstVariantPtr p_self, GDExtensionStringPtr r_ret) {
	const Variant *self = (const Variant *)p_self;
	memnew_placement(r_ret, String(*self));
}

static GDExtensionVariantType gdextension_variant_get_type(GDExtensionConstVariantPtr p_self) {
	const Variant *self = (const Variant *)p_self;
	return (GDExtensionVariantType)self->get_type();
}

static GDExtensionBool gdextension_variant_has_method(GDExtensionConstVariantPtr p_self, GDExtensionConstStringNamePtr p_method) {
	const Variant *self = (const Variant *)p_self;
	const StringName *method = (const StringName *)p_method;
	return self->has_method(*method);
}

static GDExtensionBool gdextension_variant_has_member(GDExtensionVariantType p_type, GDExtensionConstStringNamePtr p_member) {
	return Variant::has_member((Variant::Type)p_type, *((const StringName *)p_member));
}

static GDExtensionBool gdextension_variant_has_key(GDExtensionConstVariantPtr p_self, GDExtensionConstVariantPtr p_key, GDExtensionBool *r_valid) {
	const Variant *self = (const Variant *)p_self;
	const Variant *key = (const Variant *)p_key;
	bool valid;
	bool ret = self->has_key(*key, valid);
	*r_valid = valid;
	return ret;
}

static void gdextension_variant_get_type_name(GDExtensionVariantType p_type, GDExtensionStringPtr r_ret) {
	String name = Variant::get_type_name((Variant::Type)p_type);
	memnew_placement(r_ret, String(name));
}

static GDExtensionBool gdextension_variant_can_convert(GDExtensionVariantType p_from, GDExtensionVariantType p_to) {
	return Variant::can_convert((Variant::Type)p_from, (Variant::Type)p_to);
}

static GDExtensionBool gdextension_variant_can_convert_strict(GDExtensionVariantType p_from, GDExtensionVariantType p_to) {
	return Variant::can_convert_strict((Variant::Type)p_from, (Variant::Type)p_to);
}

// Variant interaction.
static GDExtensionVariantFromTypeConstructorFunc gdextension_get_variant_from_type_constructor(GDExtensionVariantType p_type) {
	switch (p_type) {
		case GDEXTENSION_VARIANT_TYPE_BOOL:
			return VariantTypeConstructor<bool>::variant_from_type;
		case GDEXTENSION_VARIANT_TYPE_INT:
			return VariantTypeConstructor<int64_t>::variant_from_type;
		case GDEXTENSION_VARIANT_TYPE_FLOAT:
			return VariantTypeConstructor<double>::variant_from_type;
		case GDEXTENSION_VARIANT_TYPE_STRING:
			return VariantTypeConstructor<String>::variant_from_type;
		case GDEXTENSION_VARIANT_TYPE_VECTOR2:
			return VariantTypeConstructor<Vector2>::variant_from_type;
		case GDEXTENSION_VARIANT_TYPE_VECTOR2I:
			return VariantTypeConstructor<Vector2i>::variant_from_type;
		case GDEXTENSION_VARIANT_TYPE_RECT2:
			return VariantTypeConstructor<Rect2>::variant_from_type;
		case GDEXTENSION_VARIANT_TYPE_RECT2I:
			return VariantTypeConstructor<Rect2i>::variant_from_type;
		case GDEXTENSION_VARIANT_TYPE_VECTOR3:
			return VariantTypeConstructor<Vector3>::variant_from_type;
		case GDEXTENSION_VARIANT_TYPE_VECTOR3I:
			return VariantTypeConstructor<Vector3i>::variant_from_type;
		case GDEXTENSION_VARIANT_TYPE_TRANSFORM2D:
			return VariantTypeConstructor<Transform2D>::variant_from_type;
		case GDEXTENSION_VARIANT_TYPE_VECTOR4:
			return VariantTypeConstructor<Vector4>::variant_from_type;
		case GDEXTENSION_VARIANT_TYPE_VECTOR4I:
			return VariantTypeConstructor<Vector4i>::variant_from_type;
		case GDEXTENSION_VARIANT_TYPE_PLANE:
			return VariantTypeConstructor<Plane>::variant_from_type;
		case GDEXTENSION_VARIANT_TYPE_QUATERNION:
			return VariantTypeConstructor<Quaternion>::variant_from_type;
		case GDEXTENSION_VARIANT_TYPE_AABB:
			return VariantTypeConstructor<AABB>::variant_from_type;
		case GDEXTENSION_VARIANT_TYPE_BASIS:
			return VariantTypeConstructor<Basis>::variant_from_type;
		case GDEXTENSION_VARIANT_TYPE_TRANSFORM3D:
			return VariantTypeConstructor<Transform3D>::variant_from_type;
		case GDEXTENSION_VARIANT_TYPE_PROJECTION:
			return VariantTypeConstructor<Projection>::variant_from_type;
		case GDEXTENSION_VARIANT_TYPE_COLOR:
			return VariantTypeConstructor<Color>::variant_from_type;
		case GDEXTENSION_VARIANT_TYPE_STRING_NAME:
			return VariantTypeConstructor<StringName>::variant_from_type;
		case GDEXTENSION_VARIANT_TYPE_NODE_PATH:
			return VariantTypeConstructor<NodePath>::variant_from_type;
		case GDEXTENSION_VARIANT_TYPE_RID:
			return VariantTypeConstructor<RID>::variant_from_type;
		case GDEXTENSION_VARIANT_TYPE_OBJECT:
			return VariantTypeConstructor<Object *>::variant_from_type;
		case GDEXTENSION_VARIANT_TYPE_CALLABLE:
			return VariantTypeConstructor<Callable>::variant_from_type;
		case GDEXTENSION_VARIANT_TYPE_SIGNAL:
			return VariantTypeConstructor<Signal>::variant_from_type;
		case GDEXTENSION_VARIANT_TYPE_DICTIONARY:
			return VariantTypeConstructor<Dictionary>::variant_from_type;
		case GDEXTENSION_VARIANT_TYPE_ARRAY:
			return VariantTypeConstructor<Array>::variant_from_type;
		case GDEXTENSION_VARIANT_TYPE_PACKED_BYTE_ARRAY:
			return VariantTypeConstructor<PackedByteArray>::variant_from_type;
		case GDEXTENSION_VARIANT_TYPE_PACKED_INT32_ARRAY:
			return VariantTypeConstructor<PackedInt32Array>::variant_from_type;
		case GDEXTENSION_VARIANT_TYPE_PACKED_INT64_ARRAY:
			return VariantTypeConstructor<PackedInt64Array>::variant_from_type;
		case GDEXTENSION_VARIANT_TYPE_PACKED_FLOAT32_ARRAY:
			return VariantTypeConstructor<PackedFloat32Array>::variant_from_type;
		case GDEXTENSION_VARIANT_TYPE_PACKED_FLOAT64_ARRAY:
			return VariantTypeConstructor<PackedFloat64Array>::variant_from_type;
		case GDEXTENSION_VARIANT_TYPE_PACKED_STRING_ARRAY:
			return VariantTypeConstructor<PackedStringArray>::variant_from_type;
		case GDEXTENSION_VARIANT_TYPE_PACKED_VECTOR2_ARRAY:
			return VariantTypeConstructor<PackedVector2Array>::variant_from_type;
		case GDEXTENSION_VARIANT_TYPE_PACKED_VECTOR3_ARRAY:
			return VariantTypeConstructor<PackedVector3Array>::variant_from_type;
		case GDEXTENSION_VARIANT_TYPE_PACKED_COLOR_ARRAY:
			return VariantTypeConstructor<PackedColorArray>::variant_from_type;
		case GDEXTENSION_VARIANT_TYPE_NIL:
		case GDEXTENSION_VARIANT_TYPE_VARIANT_MAX:
			ERR_FAIL_V_MSG(nullptr, "Getting Variant conversion function with invalid type");
	}
	ERR_FAIL_V_MSG(nullptr, "Getting Variant conversion function with invalid type");
}

static GDExtensionTypeFromVariantConstructorFunc gdextension_get_variant_to_type_constructor(GDExtensionVariantType p_type) {
	switch (p_type) {
		case GDEXTENSION_VARIANT_TYPE_BOOL:
			return VariantTypeConstructor<bool>::type_from_variant;
		case GDEXTENSION_VARIANT_TYPE_INT:
			return VariantTypeConstructor<int64_t>::type_from_variant;
		case GDEXTENSION_VARIANT_TYPE_FLOAT:
			return VariantTypeConstructor<double>::type_from_variant;
		case GDEXTENSION_VARIANT_TYPE_STRING:
			return VariantTypeConstructor<String>::type_from_variant;
		case GDEXTENSION_VARIANT_TYPE_VECTOR2:
			return VariantTypeConstructor<Vector2>::type_from_variant;
		case GDEXTENSION_VARIANT_TYPE_VECTOR2I:
			return VariantTypeConstructor<Vector2i>::type_from_variant;
		case GDEXTENSION_VARIANT_TYPE_RECT2:
			return VariantTypeConstructor<Rect2>::type_from_variant;
		case GDEXTENSION_VARIANT_TYPE_RECT2I:
			return VariantTypeConstructor<Rect2i>::type_from_variant;
		case GDEXTENSION_VARIANT_TYPE_VECTOR3:
			return VariantTypeConstructor<Vector3>::type_from_variant;
		case GDEXTENSION_VARIANT_TYPE_VECTOR3I:
			return VariantTypeConstructor<Vector3i>::type_from_variant;
		case GDEXTENSION_VARIANT_TYPE_TRANSFORM2D:
			return VariantTypeConstructor<Transform2D>::type_from_variant;
		case GDEXTENSION_VARIANT_TYPE_VECTOR4:
			return VariantTypeConstructor<Vector4>::type_from_variant;
		case GDEXTENSION_VARIANT_TYPE_VECTOR4I:
			return VariantTypeConstructor<Vector4i>::type_from_variant;
		case GDEXTENSION_VARIANT_TYPE_PLANE:
			return VariantTypeConstructor<Plane>::type_from_variant;
		case GDEXTENSION_VARIANT_TYPE_QUATERNION:
			return VariantTypeConstructor<Quaternion>::type_from_variant;
		case GDEXTENSION_VARIANT_TYPE_AABB:
			return VariantTypeConstructor<AABB>::type_from_variant;
		case GDEXTENSION_VARIANT_TYPE_BASIS:
			return VariantTypeConstructor<Basis>::type_from_variant;
		case GDEXTENSION_VARIANT_TYPE_TRANSFORM3D:
			return VariantTypeConstructor<Transform3D>::type_from_variant;
		case GDEXTENSION_VARIANT_TYPE_PROJECTION:
			return VariantTypeConstructor<Projection>::type_from_variant;
		case GDEXTENSION_VARIANT_TYPE_COLOR:
			return VariantTypeConstructor<Color>::type_from_variant;
		case GDEXTENSION_VARIANT_TYPE_STRING_NAME:
			return VariantTypeConstructor<StringName>::type_from_variant;
		case GDEXTENSION_VARIANT_TYPE_NODE_PATH:
			return VariantTypeConstructor<NodePath>::type_from_variant;
		case GDEXTENSION_VARIANT_TYPE_RID:
			return VariantTypeConstructor<RID>::type_from_variant;
		case GDEXTENSION_VARIANT_TYPE_OBJECT:
			return VariantTypeConstructor<Object *>::type_from_variant;
		case GDEXTENSION_VARIANT_TYPE_CALLABLE:
			return VariantTypeConstructor<Callable>::type_from_variant;
		case GDEXTENSION_VARIANT_TYPE_SIGNAL:
			return VariantTypeConstructor<Signal>::type_from_variant;
		case GDEXTENSION_VARIANT_TYPE_DICTIONARY:
			return VariantTypeConstructor<Dictionary>::type_from_variant;
		case GDEXTENSION_VARIANT_TYPE_ARRAY:
			return VariantTypeConstructor<Array>::type_from_variant;
		case GDEXTENSION_VARIANT_TYPE_PACKED_BYTE_ARRAY:
			return VariantTypeConstructor<PackedByteArray>::type_from_variant;
		case GDEXTENSION_VARIANT_TYPE_PACKED_INT32_ARRAY:
			return VariantTypeConstructor<PackedInt32Array>::type_from_variant;
		case GDEXTENSION_VARIANT_TYPE_PACKED_INT64_ARRAY:
			return VariantTypeConstructor<PackedInt64Array>::type_from_variant;
		case GDEXTENSION_VARIANT_TYPE_PACKED_FLOAT32_ARRAY:
			return VariantTypeConstructor<PackedFloat32Array>::type_from_variant;
		case GDEXTENSION_VARIANT_TYPE_PACKED_FLOAT64_ARRAY:
			return VariantTypeConstructor<PackedFloat64Array>::type_from_variant;
		case GDEXTENSION_VARIANT_TYPE_PACKED_STRING_ARRAY:
			return VariantTypeConstructor<PackedStringArray>::type_from_variant;
		case GDEXTENSION_VARIANT_TYPE_PACKED_VECTOR2_ARRAY:
			return VariantTypeConstructor<PackedVector2Array>::type_from_variant;
		case GDEXTENSION_VARIANT_TYPE_PACKED_VECTOR3_ARRAY:
			return VariantTypeConstructor<PackedVector3Array>::type_from_variant;
		case GDEXTENSION_VARIANT_TYPE_PACKED_COLOR_ARRAY:
			return VariantTypeConstructor<PackedColorArray>::type_from_variant;
		case GDEXTENSION_VARIANT_TYPE_NIL:
		case GDEXTENSION_VARIANT_TYPE_VARIANT_MAX:
			ERR_FAIL_V_MSG(nullptr, "Getting Variant conversion function with invalid type");
	}
	ERR_FAIL_V_MSG(nullptr, "Getting Variant conversion function with invalid type");
}

// ptrcalls
static GDExtensionPtrOperatorEvaluator gdextension_variant_get_ptr_operator_evaluator(GDExtensionVariantOperator p_operator, GDExtensionVariantType p_type_a, GDExtensionVariantType p_type_b) {
	return (GDExtensionPtrOperatorEvaluator)Variant::get_ptr_operator_evaluator(Variant::Operator(p_operator), Variant::Type(p_type_a), Variant::Type(p_type_b));
}
static GDExtensionPtrBuiltInMethod gdextension_variant_get_ptr_builtin_method(GDExtensionVariantType p_type, GDExtensionConstStringNamePtr p_method, GDExtensionInt p_hash) {
	const StringName method = *reinterpret_cast<const StringName *>(p_method);
	uint32_t hash = Variant::get_builtin_method_hash(Variant::Type(p_type), method);
	if (hash != p_hash) {
		ERR_PRINT_ONCE("Error getting method " + method + ", hash mismatch.");
		return nullptr;
	}

	return (GDExtensionPtrBuiltInMethod)Variant::get_ptr_builtin_method(Variant::Type(p_type), method);
}
static GDExtensionPtrConstructor gdextension_variant_get_ptr_constructor(GDExtensionVariantType p_type, int32_t p_constructor) {
	return (GDExtensionPtrConstructor)Variant::get_ptr_constructor(Variant::Type(p_type), p_constructor);
}
static GDExtensionPtrDestructor gdextension_variant_get_ptr_destructor(GDExtensionVariantType p_type) {
	return (GDExtensionPtrDestructor)Variant::get_ptr_destructor(Variant::Type(p_type));
}
static void gdextension_variant_construct(GDExtensionVariantType p_type, GDExtensionVariantPtr p_base, const GDExtensionConstVariantPtr *p_args, int32_t p_argument_count, GDExtensionCallError *r_error) {
	memnew_placement(p_base, Variant);

	Callable::CallError error;
	Variant::construct(Variant::Type(p_type), *(Variant *)p_base, (const Variant **)p_args, p_argument_count, error);

	if (r_error) {
		r_error->error = (GDExtensionCallErrorType)(error.error);
		r_error->argument = error.argument;
		r_error->expected = error.expected;
	}
}
static GDExtensionPtrSetter gdextension_variant_get_ptr_setter(GDExtensionVariantType p_type, GDExtensionConstStringNamePtr p_member) {
	const StringName member = *reinterpret_cast<const StringName *>(p_member);
	return (GDExtensionPtrSetter)Variant::get_member_ptr_setter(Variant::Type(p_type), member);
}
static GDExtensionPtrGetter gdextension_variant_get_ptr_getter(GDExtensionVariantType p_type, GDExtensionConstStringNamePtr p_member) {
	const StringName member = *reinterpret_cast<const StringName *>(p_member);
	return (GDExtensionPtrGetter)Variant::get_member_ptr_getter(Variant::Type(p_type), member);
}
static GDExtensionPtrIndexedSetter gdextension_variant_get_ptr_indexed_setter(GDExtensionVariantType p_type) {
	return (GDExtensionPtrIndexedSetter)Variant::get_member_ptr_indexed_setter(Variant::Type(p_type));
}
static GDExtensionPtrIndexedGetter gdextension_variant_get_ptr_indexed_getter(GDExtensionVariantType p_type) {
	return (GDExtensionPtrIndexedGetter)Variant::get_member_ptr_indexed_getter(Variant::Type(p_type));
}
static GDExtensionPtrKeyedSetter gdextension_variant_get_ptr_keyed_setter(GDExtensionVariantType p_type) {
	return (GDExtensionPtrKeyedSetter)Variant::get_member_ptr_keyed_setter(Variant::Type(p_type));
}
static GDExtensionPtrKeyedGetter gdextension_variant_get_ptr_keyed_getter(GDExtensionVariantType p_type) {
	return (GDExtensionPtrKeyedGetter)Variant::get_member_ptr_keyed_getter(Variant::Type(p_type));
}
static GDExtensionPtrKeyedChecker gdextension_variant_get_ptr_keyed_checker(GDExtensionVariantType p_type) {
	return (GDExtensionPtrKeyedChecker)Variant::get_member_ptr_keyed_checker(Variant::Type(p_type));
}
static void gdextension_variant_get_constant_value(GDExtensionVariantType p_type, GDExtensionConstStringNamePtr p_constant, GDExtensionVariantPtr r_ret) {
	StringName constant = *reinterpret_cast<const StringName *>(p_constant);
	memnew_placement(r_ret, Variant(Variant::get_constant_value(Variant::Type(p_type), constant)));
}
static GDExtensionPtrUtilityFunction gdextension_variant_get_ptr_utility_function(GDExtensionConstStringNamePtr p_function, GDExtensionInt p_hash) {
	StringName function = *reinterpret_cast<const StringName *>(p_function);
	uint32_t hash = Variant::get_utility_function_hash(function);
	if (hash != p_hash) {
		ERR_PRINT_ONCE("Error getting utility function " + function + ", hash mismatch.");
		return nullptr;
	}
	return (GDExtensionPtrUtilityFunction)Variant::get_ptr_utility_function(function);
}

//string helpers

static void gdextension_string_new_with_latin1_chars(GDExtensionStringPtr r_dest, const char *p_contents) {
	String *dest = (String *)r_dest;
	memnew_placement(dest, String);
	*dest = String(p_contents);
}

static void gdextension_string_new_with_utf8_chars(GDExtensionStringPtr r_dest, const char *p_contents) {
	String *dest = (String *)r_dest;
	memnew_placement(dest, String);
	dest->parse_utf8(p_contents);
}

static void gdextension_string_new_with_utf16_chars(GDExtensionStringPtr r_dest, const char16_t *p_contents) {
	String *dest = (String *)r_dest;
	memnew_placement(dest, String);
	dest->parse_utf16(p_contents);
}

static void gdextension_string_new_with_utf32_chars(GDExtensionStringPtr r_dest, const char32_t *p_contents) {
	String *dest = (String *)r_dest;
	memnew_placement(dest, String);
	*dest = String((const char32_t *)p_contents);
}

static void gdextension_string_new_with_wide_chars(GDExtensionStringPtr r_dest, const wchar_t *p_contents) {
	String *dest = (String *)r_dest;
	if constexpr (sizeof(wchar_t) == 2) {
		// wchar_t is 16 bit, parse.
		memnew_placement(dest, String);
		dest->parse_utf16((const char16_t *)p_contents);
	} else {
		// wchar_t is 32 bit, copy.
		memnew_placement(dest, String);
		*dest = String((const char32_t *)p_contents);
	}
}

static void gdextension_string_new_with_latin1_chars_and_len(GDExtensionStringPtr r_dest, const char *p_contents, GDExtensionInt p_size) {
	String *dest = (String *)r_dest;
	memnew_placement(dest, String);
	*dest = String(p_contents, p_size);
}

static void gdextension_string_new_with_utf8_chars_and_len(GDExtensionStringPtr r_dest, const char *p_contents, GDExtensionInt p_size) {
	String *dest = (String *)r_dest;
	memnew_placement(dest, String);
	dest->parse_utf8(p_contents, p_size);
}

static void gdextension_string_new_with_utf16_chars_and_len(GDExtensionStringPtr r_dest, const char16_t *p_contents, GDExtensionInt p_size) {
	String *dest = (String *)r_dest;
	memnew_placement(dest, String);
	dest->parse_utf16(p_contents, p_size);
}

static void gdextension_string_new_with_utf32_chars_and_len(GDExtensionStringPtr r_dest, const char32_t *p_contents, GDExtensionInt p_size) {
	String *dest = (String *)r_dest;
	memnew_placement(dest, String);
	*dest = String((const char32_t *)p_contents, p_size);
}

static void gdextension_string_new_with_wide_chars_and_len(GDExtensionStringPtr r_dest, const wchar_t *p_contents, GDExtensionInt p_size) {
	String *dest = (String *)r_dest;
	if constexpr (sizeof(wchar_t) == 2) {
		// wchar_t is 16 bit, parse.
		memnew_placement(dest, String);
		dest->parse_utf16((const char16_t *)p_contents, p_size);
	} else {
		// wchar_t is 32 bit, copy.
		memnew_placement(dest, String);
		*dest = String((const char32_t *)p_contents, p_size);
	}
}

static GDExtensionInt gdextension_string_to_latin1_chars(GDExtensionConstStringPtr p_self, char *r_text, GDExtensionInt p_max_write_length) {
	String *self = (String *)p_self;
	CharString cs = self->ascii(true);
	GDExtensionInt len = cs.length();
	if (r_text) {
		const char *s_text = cs.ptr();
		for (GDExtensionInt i = 0; i < MIN(len, p_max_write_length); i++) {
			r_text[i] = s_text[i];
		}
	}
	return len;
}
static GDExtensionInt gdextension_string_to_utf8_chars(GDExtensionConstStringPtr p_self, char *r_text, GDExtensionInt p_max_write_length) {
	String *self = (String *)p_self;
	CharString cs = self->utf8();
	GDExtensionInt len = cs.length();
	if (r_text) {
		const char *s_text = cs.ptr();
		for (GDExtensionInt i = 0; i < MIN(len, p_max_write_length); i++) {
			r_text[i] = s_text[i];
		}
	}
	return len;
}
static GDExtensionInt gdextension_string_to_utf16_chars(GDExtensionConstStringPtr p_self, char16_t *r_text, GDExtensionInt p_max_write_length) {
	String *self = (String *)p_self;
	Char16String cs = self->utf16();
	GDExtensionInt len = cs.length();
	if (r_text) {
		const char16_t *s_text = cs.ptr();
		for (GDExtensionInt i = 0; i < MIN(len, p_max_write_length); i++) {
			r_text[i] = s_text[i];
		}
	}
	return len;
}
static GDExtensionInt gdextension_string_to_utf32_chars(GDExtensionConstStringPtr p_self, char32_t *r_text, GDExtensionInt p_max_write_length) {
	String *self = (String *)p_self;
	GDExtensionInt len = self->length();
	if (r_text) {
		const char32_t *s_text = self->ptr();
		for (GDExtensionInt i = 0; i < MIN(len, p_max_write_length); i++) {
			r_text[i] = s_text[i];
		}
	}
	return len;
}
static GDExtensionInt gdextension_string_to_wide_chars(GDExtensionConstStringPtr p_self, wchar_t *r_text, GDExtensionInt p_max_write_length) {
	if constexpr (sizeof(wchar_t) == 4) {
		return gdextension_string_to_utf32_chars(p_self, (char32_t *)r_text, p_max_write_length);
	} else {
		return gdextension_string_to_utf16_chars(p_self, (char16_t *)r_text, p_max_write_length);
	}
}

static char32_t *gdextension_string_operator_index(GDExtensionStringPtr p_self, GDExtensionInt p_index) {
	String *self = (String *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->length() + 1, nullptr);
	return &self->ptrw()[p_index];
}

static const char32_t *gdextension_string_operator_index_const(GDExtensionConstStringPtr p_self, GDExtensionInt p_index) {
	const String *self = (const String *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->length() + 1, nullptr);
	return &self->ptr()[p_index];
}

static void gdextension_string_operator_plus_eq_string(GDExtensionStringPtr p_self, GDExtensionConstStringPtr p_b) {
	String *self = (String *)p_self;
	const String *b = (const String *)p_b;
	*self += *b;
}

static void gdextension_string_operator_plus_eq_char(GDExtensionStringPtr p_self, char32_t p_b) {
	String *self = (String *)p_self;
	*self += p_b;
}

static void gdextension_string_operator_plus_eq_cstr(GDExtensionStringPtr p_self, const char *p_b) {
	String *self = (String *)p_self;
	*self += p_b;
}

static void gdextension_string_operator_plus_eq_wcstr(GDExtensionStringPtr p_self, const wchar_t *p_b) {
	String *self = (String *)p_self;
	*self += p_b;
}

static void gdextension_string_operator_plus_eq_c32str(GDExtensionStringPtr p_self, const char32_t *p_b) {
	String *self = (String *)p_self;
	*self += p_b;
}

static GDExtensionInt gdextension_xml_parser_open_buffer(GDExtensionObjectPtr p_instance, const uint8_t *p_buffer, size_t p_size) {
	XMLParser *xml = (XMLParser *)p_instance;
	return (GDExtensionInt)xml->_open_buffer(p_buffer, p_size);
}

static void gdextension_file_access_store_buffer(GDExtensionObjectPtr p_instance, const uint8_t *p_src, uint64_t p_length) {
	FileAccess *fa = (FileAccess *)p_instance;
	fa->store_buffer(p_src, p_length);
}

static uint64_t gdextension_file_access_get_buffer(GDExtensionConstObjectPtr p_instance, uint8_t *p_dst, uint64_t p_length) {
	const FileAccess *fa = (FileAccess *)p_instance;
	return fa->get_buffer(p_dst, p_length);
}

static int64_t gdextension_worker_thread_pool_add_native_group_task(GDExtensionObjectPtr p_instance, void (*p_func)(void *, uint32_t), void *p_userdata, int p_elements, int p_tasks, GDExtensionBool p_high_priority, GDExtensionConstStringPtr p_description) {
	WorkerThreadPool *p = (WorkerThreadPool *)p_instance;
	const String *description = (const String *)p_description;
	return (int64_t)p->add_native_group_task(p_func, p_userdata, p_elements, p_tasks, static_cast<bool>(p_high_priority), *description);
}

static int64_t gdextension_worker_thread_pool_add_native_task(GDExtensionObjectPtr p_instance, void (*p_func)(void *), void *p_userdata, GDExtensionBool p_high_priority, GDExtensionConstStringPtr p_description) {
	WorkerThreadPool *p = (WorkerThreadPool *)p_instance;
	const String *description = (const String *)p_description;
	return (int64_t)p->add_native_task(p_func, p_userdata, static_cast<bool>(p_high_priority), *description);
}

/* Packed array functions */

static uint8_t *gdextension_packed_byte_array_operator_index(GDExtensionTypePtr p_self, GDExtensionInt p_index) {
	PackedByteArray *self = (PackedByteArray *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return &self->ptrw()[p_index];
}

static const uint8_t *gdextension_packed_byte_array_operator_index_const(GDExtensionConstTypePtr p_self, GDExtensionInt p_index) {
	const PackedByteArray *self = (const PackedByteArray *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return &self->ptr()[p_index];
}

static GDExtensionTypePtr gdextension_packed_color_array_operator_index(GDExtensionTypePtr p_self, GDExtensionInt p_index) {
	PackedColorArray *self = (PackedColorArray *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return (GDExtensionTypePtr)&self->ptrw()[p_index];
}

static GDExtensionTypePtr gdextension_packed_color_array_operator_index_const(GDExtensionConstTypePtr p_self, GDExtensionInt p_index) {
	const PackedColorArray *self = (const PackedColorArray *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return (GDExtensionTypePtr)&self->ptr()[p_index];
}

static float *gdextension_packed_float32_array_operator_index(GDExtensionTypePtr p_self, GDExtensionInt p_index) {
	PackedFloat32Array *self = (PackedFloat32Array *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return &self->ptrw()[p_index];
}

static const float *gdextension_packed_float32_array_operator_index_const(GDExtensionConstTypePtr p_self, GDExtensionInt p_index) {
	const PackedFloat32Array *self = (const PackedFloat32Array *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return &self->ptr()[p_index];
}

static double *gdextension_packed_float64_array_operator_index(GDExtensionTypePtr p_self, GDExtensionInt p_index) {
	PackedFloat64Array *self = (PackedFloat64Array *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return &self->ptrw()[p_index];
}

static const double *gdextension_packed_float64_array_operator_index_const(GDExtensionConstTypePtr p_self, GDExtensionInt p_index) {
	const PackedFloat64Array *self = (const PackedFloat64Array *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return &self->ptr()[p_index];
}

static int32_t *gdextension_packed_int32_array_operator_index(GDExtensionTypePtr p_self, GDExtensionInt p_index) {
	PackedInt32Array *self = (PackedInt32Array *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return &self->ptrw()[p_index];
}

static const int32_t *gdextension_packed_int32_array_operator_index_const(GDExtensionConstTypePtr p_self, GDExtensionInt p_index) {
	const PackedInt32Array *self = (const PackedInt32Array *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return &self->ptr()[p_index];
}

static int64_t *gdextension_packed_int64_array_operator_index(GDExtensionTypePtr p_self, GDExtensionInt p_index) {
	PackedInt64Array *self = (PackedInt64Array *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return &self->ptrw()[p_index];
}

static const int64_t *gdextension_packed_int64_array_operator_index_const(GDExtensionConstTypePtr p_self, GDExtensionInt p_index) {
	const PackedInt64Array *self = (const PackedInt64Array *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return &self->ptr()[p_index];
}

static GDExtensionStringPtr gdextension_packed_string_array_operator_index(GDExtensionTypePtr p_self, GDExtensionInt p_index) {
	PackedStringArray *self = (PackedStringArray *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return (GDExtensionStringPtr)&self->ptrw()[p_index];
}

static GDExtensionStringPtr gdextension_packed_string_array_operator_index_const(GDExtensionConstTypePtr p_self, GDExtensionInt p_index) {
	const PackedStringArray *self = (const PackedStringArray *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return (GDExtensionStringPtr)&self->ptr()[p_index];
}

static GDExtensionTypePtr gdextension_packed_vector2_array_operator_index(GDExtensionTypePtr p_self, GDExtensionInt p_index) {
	PackedVector2Array *self = (PackedVector2Array *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return (GDExtensionTypePtr)&self->ptrw()[p_index];
}

static GDExtensionTypePtr gdextension_packed_vector2_array_operator_index_const(GDExtensionConstTypePtr p_self, GDExtensionInt p_index) {
	const PackedVector2Array *self = (const PackedVector2Array *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return (GDExtensionTypePtr)&self->ptr()[p_index];
}

static GDExtensionTypePtr gdextension_packed_vector3_array_operator_index(GDExtensionTypePtr p_self, GDExtensionInt p_index) {
	PackedVector3Array *self = (PackedVector3Array *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return (GDExtensionTypePtr)&self->ptrw()[p_index];
}

static GDExtensionTypePtr gdextension_packed_vector3_array_operator_index_const(GDExtensionConstTypePtr p_self, GDExtensionInt p_index) {
	const PackedVector3Array *self = (const PackedVector3Array *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return (GDExtensionTypePtr)&self->ptr()[p_index];
}

static GDExtensionVariantPtr gdextension_array_operator_index(GDExtensionTypePtr p_self, GDExtensionInt p_index) {
	Array *self = (Array *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return (GDExtensionVariantPtr)&self->operator[](p_index);
}

static GDExtensionVariantPtr gdextension_array_operator_index_const(GDExtensionConstTypePtr p_self, GDExtensionInt p_index) {
	const Array *self = (const Array *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return (GDExtensionVariantPtr)&self->operator[](p_index);
}

void gdextension_array_ref(GDExtensionTypePtr p_self, GDExtensionConstTypePtr p_from) {
	Array *self = (Array *)p_self;
	const Array *from = (const Array *)p_from;
	self->_ref(*from);
}

void gdextension_array_set_typed(GDExtensionTypePtr p_self, GDExtensionVariantType p_type, GDExtensionConstStringNamePtr p_class_name, GDExtensionConstVariantPtr p_script) {
	Array *self = reinterpret_cast<Array *>(p_self);
	const StringName *class_name = reinterpret_cast<const StringName *>(p_class_name);
	const Variant *script = reinterpret_cast<const Variant *>(p_script);
	self->set_typed((uint32_t)p_type, *class_name, *script);
}

/* Dictionary functions */

static GDExtensionVariantPtr gdextension_dictionary_operator_index(GDExtensionTypePtr p_self, GDExtensionConstVariantPtr p_key) {
	Dictionary *self = (Dictionary *)p_self;
	return (GDExtensionVariantPtr)&self->operator[](*(const Variant *)p_key);
}

static GDExtensionVariantPtr gdextension_dictionary_operator_index_const(GDExtensionConstTypePtr p_self, GDExtensionConstVariantPtr p_key) {
	const Dictionary *self = (const Dictionary *)p_self;
	return (GDExtensionVariantPtr)&self->operator[](*(const Variant *)p_key);
}

/* OBJECT API */

static void gdextension_object_method_bind_call(GDExtensionMethodBindPtr p_method_bind, GDExtensionObjectPtr p_instance, const GDExtensionConstVariantPtr *p_args, GDExtensionInt p_arg_count, GDExtensionVariantPtr r_return, GDExtensionCallError *r_error) {
	const MethodBind *mb = reinterpret_cast<const MethodBind *>(p_method_bind);
	Object *o = (Object *)p_instance;
	const Variant **args = (const Variant **)p_args;
	Callable::CallError error;

	Variant ret = mb->call(o, args, p_arg_count, error);
	memnew_placement(r_return, Variant(ret));

	if (r_error) {
		r_error->error = (GDExtensionCallErrorType)(error.error);
		r_error->argument = error.argument;
		r_error->expected = error.expected;
	}
}

static void gdextension_object_method_bind_ptrcall(GDExtensionMethodBindPtr p_method_bind, GDExtensionObjectPtr p_instance, const GDExtensionConstTypePtr *p_args, GDExtensionTypePtr p_ret) {
	const MethodBind *mb = reinterpret_cast<const MethodBind *>(p_method_bind);
	Object *o = (Object *)p_instance;
	mb->ptrcall(o, (const void **)p_args, p_ret);
}

static void gdextension_object_destroy(GDExtensionObjectPtr p_o) {
	memdelete((Object *)p_o);
}

static GDExtensionObjectPtr gdextension_global_get_singleton(GDExtensionConstStringNamePtr p_name) {
	const StringName name = *reinterpret_cast<const StringName *>(p_name);
	return (GDExtensionObjectPtr)Engine::get_singleton()->get_singleton_object(name);
}

static void *gdextension_object_get_instance_binding(GDExtensionObjectPtr p_object, void *p_token, const GDExtensionInstanceBindingCallbacks *p_callbacks) {
	Object *o = (Object *)p_object;
	return o->get_instance_binding(p_token, p_callbacks);
}

static void gdextension_object_set_instance_binding(GDExtensionObjectPtr p_object, void *p_token, void *p_binding, const GDExtensionInstanceBindingCallbacks *p_callbacks) {
	Object *o = (Object *)p_object;
	o->set_instance_binding(p_token, p_binding, p_callbacks);
}

static void gdextension_object_set_instance(GDExtensionObjectPtr p_object, GDExtensionConstStringNamePtr p_classname, GDExtensionClassInstancePtr p_instance) {
	const StringName classname = *reinterpret_cast<const StringName *>(p_classname);
	Object *o = (Object *)p_object;
	ClassDB::set_object_extension_instance(o, classname, p_instance);
}

static GDExtensionObjectPtr gdextension_object_get_instance_from_id(GDObjectInstanceID p_instance_id) {
	return (GDExtensionObjectPtr)ObjectDB::get_instance(ObjectID(p_instance_id));
}

static GDExtensionObjectPtr gdextension_object_cast_to(GDExtensionConstObjectPtr p_object, void *p_class_tag) {
	if (!p_object) {
		return nullptr;
	}
	Object *o = (Object *)p_object;

	return o->is_class_ptr(p_class_tag) ? (GDExtensionObjectPtr)o : (GDExtensionObjectPtr) nullptr;
}

static GDObjectInstanceID gdextension_object_get_instance_id(GDExtensionConstObjectPtr p_object) {
	const Object *o = (const Object *)p_object;
	return (GDObjectInstanceID)o->get_instance_id();
}

static GDExtensionObjectPtr gdextension_ref_get_object(GDExtensionConstRefPtr p_ref) {
	const Ref<RefCounted> *ref = (const Ref<RefCounted> *)p_ref;
	if (ref == nullptr || ref->is_null()) {
		return (GDExtensionObjectPtr) nullptr;
	} else {
		return (GDExtensionObjectPtr)ref->ptr();
	}
}

static void gdextension_ref_set_object(GDExtensionRefPtr p_ref, GDExtensionObjectPtr p_object) {
	Ref<RefCounted> *ref = (Ref<RefCounted> *)p_ref;
	ERR_FAIL_NULL(ref);

	Object *o = (RefCounted *)p_object;
	ref->reference_ptr(o);
}

static GDExtensionScriptInstancePtr gdextension_script_instance_create(const GDExtensionScriptInstanceInfo *p_info, GDExtensionScriptInstanceDataPtr p_instance_data) {
	ScriptInstanceExtension *script_instance_extension = memnew(ScriptInstanceExtension);
	script_instance_extension->instance = p_instance_data;
	script_instance_extension->native_info = p_info;
	return reinterpret_cast<GDExtensionScriptInstancePtr>(script_instance_extension);
}

static GDExtensionMethodBindPtr gdextension_classdb_get_method_bind(GDExtensionConstStringNamePtr p_classname, GDExtensionConstStringNamePtr p_methodname, GDExtensionInt p_hash) {
	const StringName classname = *reinterpret_cast<const StringName *>(p_classname);
	const StringName methodname = *reinterpret_cast<const StringName *>(p_methodname);
	MethodBind *mb = ClassDB::get_method(classname, methodname);
	ERR_FAIL_COND_V(!mb, nullptr);
	if (mb->get_hash() != p_hash) {
		ERR_PRINT("Hash mismatch for method '" + classname + "." + methodname + "'.");
		return nullptr;
	}
	return (GDExtensionMethodBindPtr)mb;
}

static GDExtensionObjectPtr gdextension_classdb_construct_object(GDExtensionConstStringNamePtr p_classname) {
	const StringName classname = *reinterpret_cast<const StringName *>(p_classname);
	return (GDExtensionObjectPtr)ClassDB::instantiate(classname);
}

static void *gdextension_classdb_get_class_tag(GDExtensionConstStringNamePtr p_classname) {
	const StringName classname = *reinterpret_cast<const StringName *>(p_classname);
	ClassDB::ClassInfo *class_info = ClassDB::classes.getptr(classname);
	return class_info ? class_info->class_ptr : nullptr;
}

#define REGISTER_INTERFACE_FUNC(m_name) GDExtension::register_interface_function(#m_name, (GDExtensionInterfaceFunctionPtr)&gdextension_##m_name)

void gdextension_setup_interface() {
	REGISTER_INTERFACE_FUNC(get_godot_version);
	REGISTER_INTERFACE_FUNC(mem_alloc);
	REGISTER_INTERFACE_FUNC(mem_realloc);
	REGISTER_INTERFACE_FUNC(mem_free);
	REGISTER_INTERFACE_FUNC(print_error);
	REGISTER_INTERFACE_FUNC(print_error_with_message);
	REGISTER_INTERFACE_FUNC(print_warning);
	REGISTER_INTERFACE_FUNC(print_warning_with_message);
	REGISTER_INTERFACE_FUNC(print_script_error);
	REGISTER_INTERFACE_FUNC(print_script_error_with_message);
	REGISTER_INTERFACE_FUNC(get_native_struct_size);
	REGISTER_INTERFACE_FUNC(variant_new_copy);
	REGISTER_INTERFACE_FUNC(variant_new_nil);
	REGISTER_INTERFACE_FUNC(variant_destroy);
	REGISTER_INTERFACE_FUNC(variant_call);
	REGISTER_INTERFACE_FUNC(variant_call_static);
	REGISTER_INTERFACE_FUNC(variant_evaluate);
	REGISTER_INTERFACE_FUNC(variant_set);
	REGISTER_INTERFACE_FUNC(variant_set_named);
	REGISTER_INTERFACE_FUNC(variant_set_keyed);
	REGISTER_INTERFACE_FUNC(variant_set_indexed);
	REGISTER_INTERFACE_FUNC(variant_get);
	REGISTER_INTERFACE_FUNC(variant_get_named);
	REGISTER_INTERFACE_FUNC(variant_get_keyed);
	REGISTER_INTERFACE_FUNC(variant_get_indexed);
	REGISTER_INTERFACE_FUNC(variant_iter_init);
	REGISTER_INTERFACE_FUNC(variant_iter_next);
	REGISTER_INTERFACE_FUNC(variant_iter_get);
	REGISTER_INTERFACE_FUNC(variant_hash);
	REGISTER_INTERFACE_FUNC(variant_recursive_hash);
	REGISTER_INTERFACE_FUNC(variant_hash_compare);
	REGISTER_INTERFACE_FUNC(variant_booleanize);
	REGISTER_INTERFACE_FUNC(variant_duplicate);
	REGISTER_INTERFACE_FUNC(variant_stringify);
	REGISTER_INTERFACE_FUNC(variant_get_type);
	REGISTER_INTERFACE_FUNC(variant_has_method);
	REGISTER_INTERFACE_FUNC(variant_has_member);
	REGISTER_INTERFACE_FUNC(variant_has_key);
	REGISTER_INTERFACE_FUNC(variant_get_type_name);
	REGISTER_INTERFACE_FUNC(variant_can_convert);
	REGISTER_INTERFACE_FUNC(variant_can_convert_strict);
	REGISTER_INTERFACE_FUNC(get_variant_from_type_constructor);
	REGISTER_INTERFACE_FUNC(get_variant_to_type_constructor);
	REGISTER_INTERFACE_FUNC(variant_get_ptr_operator_evaluator);
	REGISTER_INTERFACE_FUNC(variant_get_ptr_builtin_method);
	REGISTER_INTERFACE_FUNC(variant_get_ptr_constructor);
	REGISTER_INTERFACE_FUNC(variant_get_ptr_destructor);
	REGISTER_INTERFACE_FUNC(variant_construct);
	REGISTER_INTERFACE_FUNC(variant_get_ptr_setter);
	REGISTER_INTERFACE_FUNC(variant_get_ptr_getter);
	REGISTER_INTERFACE_FUNC(variant_get_ptr_indexed_setter);
	REGISTER_INTERFACE_FUNC(variant_get_ptr_indexed_getter);
	REGISTER_INTERFACE_FUNC(variant_get_ptr_keyed_setter);
	REGISTER_INTERFACE_FUNC(variant_get_ptr_keyed_getter);
	REGISTER_INTERFACE_FUNC(variant_get_ptr_keyed_checker);
	REGISTER_INTERFACE_FUNC(variant_get_constant_value);
	REGISTER_INTERFACE_FUNC(variant_get_ptr_utility_function);
	REGISTER_INTERFACE_FUNC(string_new_with_latin1_chars);
	REGISTER_INTERFACE_FUNC(string_new_with_utf8_chars);
	REGISTER_INTERFACE_FUNC(string_new_with_utf16_chars);
	REGISTER_INTERFACE_FUNC(string_new_with_utf32_chars);
	REGISTER_INTERFACE_FUNC(string_new_with_wide_chars);
	REGISTER_INTERFACE_FUNC(string_new_with_latin1_chars_and_len);
	REGISTER_INTERFACE_FUNC(string_new_with_utf8_chars_and_len);
	REGISTER_INTERFACE_FUNC(string_new_with_utf16_chars_and_len);
	REGISTER_INTERFACE_FUNC(string_new_with_utf32_chars_and_len);
	REGISTER_INTERFACE_FUNC(string_new_with_wide_chars_and_len);
	REGISTER_INTERFACE_FUNC(string_to_latin1_chars);
	REGISTER_INTERFACE_FUNC(string_to_utf8_chars);
	REGISTER_INTERFACE_FUNC(string_to_utf16_chars);
	REGISTER_INTERFACE_FUNC(string_to_utf32_chars);
	REGISTER_INTERFACE_FUNC(string_to_wide_chars);
	REGISTER_INTERFACE_FUNC(string_operator_index);
	REGISTER_INTERFACE_FUNC(string_operator_index_const);
	REGISTER_INTERFACE_FUNC(string_operator_plus_eq_string);
	REGISTER_INTERFACE_FUNC(string_operator_plus_eq_char);
	REGISTER_INTERFACE_FUNC(string_operator_plus_eq_cstr);
	REGISTER_INTERFACE_FUNC(string_operator_plus_eq_wcstr);
	REGISTER_INTERFACE_FUNC(string_operator_plus_eq_c32str);
	REGISTER_INTERFACE_FUNC(xml_parser_open_buffer);
	REGISTER_INTERFACE_FUNC(file_access_store_buffer);
	REGISTER_INTERFACE_FUNC(file_access_get_buffer);
	REGISTER_INTERFACE_FUNC(worker_thread_pool_add_native_group_task);
	REGISTER_INTERFACE_FUNC(worker_thread_pool_add_native_task);
	REGISTER_INTERFACE_FUNC(packed_byte_array_operator_index);
	REGISTER_INTERFACE_FUNC(packed_byte_array_operator_index_const);
	REGISTER_INTERFACE_FUNC(packed_color_array_operator_index);
	REGISTER_INTERFACE_FUNC(packed_color_array_operator_index_const);
	REGISTER_INTERFACE_FUNC(packed_float32_array_operator_index);
	REGISTER_INTERFACE_FUNC(packed_float32_array_operator_index_const);
	REGISTER_INTERFACE_FUNC(packed_float64_array_operator_index);
	REGISTER_INTERFACE_FUNC(packed_float64_array_operator_index_const);
	REGISTER_INTERFACE_FUNC(packed_int32_array_operator_index);
	REGISTER_INTERFACE_FUNC(packed_int32_array_operator_index_const);
	REGISTER_INTERFACE_FUNC(packed_int64_array_operator_index);
	REGISTER_INTERFACE_FUNC(packed_int64_array_operator_index_const);
	REGISTER_INTERFACE_FUNC(packed_string_array_operator_index);
	REGISTER_INTERFACE_FUNC(packed_string_array_operator_index_const);
	REGISTER_INTERFACE_FUNC(packed_vector2_array_operator_index);
	REGISTER_INTERFACE_FUNC(packed_vector2_array_operator_index_const);
	REGISTER_INTERFACE_FUNC(packed_vector3_array_operator_index);
	REGISTER_INTERFACE_FUNC(packed_vector3_array_operator_index_const);
	REGISTER_INTERFACE_FUNC(array_operator_index);
	REGISTER_INTERFACE_FUNC(array_operator_index_const);
	REGISTER_INTERFACE_FUNC(array_ref);
	REGISTER_INTERFACE_FUNC(array_set_typed);
	REGISTER_INTERFACE_FUNC(dictionary_operator_index);
	REGISTER_INTERFACE_FUNC(dictionary_operator_index_const);
	REGISTER_INTERFACE_FUNC(object_method_bind_call);
	REGISTER_INTERFACE_FUNC(object_method_bind_ptrcall);
	REGISTER_INTERFACE_FUNC(object_destroy);
	REGISTER_INTERFACE_FUNC(global_get_singleton);
	REGISTER_INTERFACE_FUNC(object_get_instance_binding);
	REGISTER_INTERFACE_FUNC(object_set_instance_binding);
	REGISTER_INTERFACE_FUNC(object_set_instance);
	REGISTER_INTERFACE_FUNC(object_cast_to);
	REGISTER_INTERFACE_FUNC(object_get_instance_from_id);
	REGISTER_INTERFACE_FUNC(object_get_instance_id);
	REGISTER_INTERFACE_FUNC(ref_get_object);
	REGISTER_INTERFACE_FUNC(ref_set_object);
	REGISTER_INTERFACE_FUNC(script_instance_create);
	REGISTER_INTERFACE_FUNC(classdb_construct_object);
	REGISTER_INTERFACE_FUNC(classdb_get_method_bind);
	REGISTER_INTERFACE_FUNC(classdb_get_class_tag);
}

#undef REGISTER_INTERFACE_FUNCTION

/*
 * Handle legacy GDExtension interface from Godot 4.0.
 */

typedef struct {
	uint32_t version_major;
	uint32_t version_minor;
	uint32_t version_patch;
	const char *version_string;

	GDExtensionInterfaceMemAlloc mem_alloc;
	GDExtensionInterfaceMemRealloc mem_realloc;
	GDExtensionInterfaceMemFree mem_free;

	GDExtensionInterfacePrintError print_error;
	GDExtensionInterfacePrintErrorWithMessage print_error_with_message;
	GDExtensionInterfacePrintWarning print_warning;
	GDExtensionInterfacePrintWarningWithMessage print_warning_with_message;
	GDExtensionInterfacePrintScriptError print_script_error;
	GDExtensionInterfacePrintScriptErrorWithMessage print_script_error_with_message;

	GDExtensionInterfaceGetNativeStructSize get_native_struct_size;

	GDExtensionInterfaceVariantNewCopy variant_new_copy;
	GDExtensionInterfaceVariantNewNil variant_new_nil;
	GDExtensionInterfaceVariantDestroy variant_destroy;

	GDExtensionInterfaceVariantCall variant_call;
	GDExtensionInterfaceVariantCallStatic variant_call_static;
	GDExtensionInterfaceVariantEvaluate variant_evaluate;
	GDExtensionInterfaceVariantSet variant_set;
	GDExtensionInterfaceVariantSetNamed variant_set_named;
	GDExtensionInterfaceVariantSetKeyed variant_set_keyed;
	GDExtensionInterfaceVariantSetIndexed variant_set_indexed;
	GDExtensionInterfaceVariantGet variant_get;
	GDExtensionInterfaceVariantGetNamed variant_get_named;
	GDExtensionInterfaceVariantGetKeyed variant_get_keyed;
	GDExtensionInterfaceVariantGetIndexed variant_get_indexed;
	GDExtensionInterfaceVariantIterInit variant_iter_init;
	GDExtensionInterfaceVariantIterNext variant_iter_next;
	GDExtensionInterfaceVariantIterGet variant_iter_get;
	GDExtensionInterfaceVariantHash variant_hash;
	GDExtensionInterfaceVariantRecursiveHash variant_recursive_hash;
	GDExtensionInterfaceVariantHashCompare variant_hash_compare;
	GDExtensionInterfaceVariantBooleanize variant_booleanize;
	GDExtensionInterfaceVariantDuplicate variant_duplicate;
	GDExtensionInterfaceVariantStringify variant_stringify;

	GDExtensionInterfaceVariantGetType variant_get_type;
	GDExtensionInterfaceVariantHasMethod variant_has_method;
	GDExtensionInterfaceVariantHasMember variant_has_member;
	GDExtensionInterfaceVariantHasKey variant_has_key;
	GDExtensionInterfaceVariantGetTypeName variant_get_type_name;
	GDExtensionInterfaceVariantCanConvert variant_can_convert;
	GDExtensionInterfaceVariantCanConvertStrict variant_can_convert_strict;

	GDExtensionInterfaceGetVariantFromTypeConstructor get_variant_from_type_constructor;
	GDExtensionInterfaceGetVariantToTypeConstructor get_variant_to_type_constructor;
	GDExtensionInterfaceVariantGetPtrOperatorEvaluator variant_get_ptr_operator_evaluator;
	GDExtensionInterfaceVariantGetPtrBuiltinMethod variant_get_ptr_builtin_method;
	GDExtensionInterfaceVariantGetPtrConstructor variant_get_ptr_constructor;
	GDExtensionInterfaceVariantGetPtrDestructor variant_get_ptr_destructor;
	GDExtensionInterfaceVariantConstruct variant_construct;
	GDExtensionInterfaceVariantGetPtrSetter variant_get_ptr_setter;
	GDExtensionInterfaceVariantGetPtrGetter variant_get_ptr_getter;
	GDExtensionInterfaceVariantGetPtrIndexedSetter variant_get_ptr_indexed_setter;
	GDExtensionInterfaceVariantGetPtrIndexedGetter variant_get_ptr_indexed_getter;
	GDExtensionInterfaceVariantGetPtrKeyedSetter variant_get_ptr_keyed_setter;
	GDExtensionInterfaceVariantGetPtrKeyedGetter variant_get_ptr_keyed_getter;
	GDExtensionInterfaceVariantGetPtrKeyedChecker variant_get_ptr_keyed_checker;
	GDExtensionInterfaceVariantGetConstantValue variant_get_constant_value;
	GDExtensionInterfaceVariantGetPtrUtilityFunction variant_get_ptr_utility_function;

	GDExtensionInterfaceStringNewWithLatin1Chars string_new_with_latin1_chars;
	GDExtensionInterfaceStringNewWithUtf8Chars string_new_with_utf8_chars;
	GDExtensionInterfaceStringNewWithUtf16Chars string_new_with_utf16_chars;
	GDExtensionInterfaceStringNewWithUtf32Chars string_new_with_utf32_chars;
	GDExtensionInterfaceStringNewWithWideChars string_new_with_wide_chars;
	GDExtensionInterfaceStringNewWithLatin1CharsAndLen string_new_with_latin1_chars_and_len;
	GDExtensionInterfaceStringNewWithUtf8CharsAndLen string_new_with_utf8_chars_and_len;
	GDExtensionInterfaceStringNewWithUtf16CharsAndLen string_new_with_utf16_chars_and_len;
	GDExtensionInterfaceStringNewWithUtf32CharsAndLen string_new_with_utf32_chars_and_len;
	GDExtensionInterfaceStringNewWithWideCharsAndLen string_new_with_wide_chars_and_len;
	GDExtensionInterfaceStringToLatin1Chars string_to_latin1_chars;
	GDExtensionInterfaceStringToUtf8Chars string_to_utf8_chars;
	GDExtensionInterfaceStringToUtf16Chars string_to_utf16_chars;
	GDExtensionInterfaceStringToUtf32Chars string_to_utf32_chars;
	GDExtensionInterfaceStringToWideChars string_to_wide_chars;
	GDExtensionInterfaceStringOperatorIndex string_operator_index;
	GDExtensionInterfaceStringOperatorIndexConst string_operator_index_const;

	GDExtensionInterfaceStringOperatorPlusEqString string_operator_plus_eq_string;
	GDExtensionInterfaceStringOperatorPlusEqChar string_operator_plus_eq_char;
	GDExtensionInterfaceStringOperatorPlusEqCstr string_operator_plus_eq_cstr;
	GDExtensionInterfaceStringOperatorPlusEqWcstr string_operator_plus_eq_wcstr;
	GDExtensionInterfaceStringOperatorPlusEqC32str string_operator_plus_eq_c32str;

	GDExtensionInterfaceXmlParserOpenBuffer xml_parser_open_buffer;

	GDExtensionInterfaceFileAccessStoreBuffer file_access_store_buffer;
	GDExtensionInterfaceFileAccessGetBuffer file_access_get_buffer;

	GDExtensionInterfaceWorkerThreadPoolAddNativeGroupTask worker_thread_pool_add_native_group_task;
	GDExtensionInterfaceWorkerThreadPoolAddNativeTask worker_thread_pool_add_native_task;

	GDExtensionInterfacePackedByteArrayOperatorIndex packed_byte_array_operator_index;
	GDExtensionInterfacePackedByteArrayOperatorIndexConst packed_byte_array_operator_index_const;
	GDExtensionInterfacePackedColorArrayOperatorIndex packed_color_array_operator_index;
	GDExtensionInterfacePackedColorArrayOperatorIndexConst packed_color_array_operator_index_const;
	GDExtensionInterfacePackedFloat32ArrayOperatorIndex packed_float32_array_operator_index;
	GDExtensionInterfacePackedFloat32ArrayOperatorIndexConst packed_float32_array_operator_index_const;
	GDExtensionInterfacePackedFloat64ArrayOperatorIndex packed_float64_array_operator_index;
	GDExtensionInterfacePackedFloat64ArrayOperatorIndexConst packed_float64_array_operator_index_const;
	GDExtensionInterfacePackedInt32ArrayOperatorIndex packed_int32_array_operator_index;
	GDExtensionInterfacePackedInt32ArrayOperatorIndexConst packed_int32_array_operator_index_const;
	GDExtensionInterfacePackedInt64ArrayOperatorIndex packed_int64_array_operator_index;
	GDExtensionInterfacePackedInt64ArrayOperatorIndexConst packed_int64_array_operator_index_const;
	GDExtensionInterfacePackedStringArrayOperatorIndex packed_string_array_operator_index;
	GDExtensionInterfacePackedStringArrayOperatorIndexConst packed_string_array_operator_index_const;
	GDExtensionInterfacePackedVector2ArrayOperatorIndex packed_vector2_array_operator_index;
	GDExtensionInterfacePackedVector2ArrayOperatorIndexConst packed_vector2_array_operator_index_const;
	GDExtensionInterfacePackedVector3ArrayOperatorIndex packed_vector3_array_operator_index;
	GDExtensionInterfacePackedVector3ArrayOperatorIndexConst packed_vector3_array_operator_index_const;
	GDExtensionInterfaceArrayOperatorIndex array_operator_index;
	GDExtensionInterfaceArrayOperatorIndexConst array_operator_index_const;
	GDExtensionInterfaceArrayRef array_ref;
	GDExtensionInterfaceArraySetTyped array_set_typed;

	GDExtensionInterfaceDictionaryOperatorIndex dictionary_operator_index;
	GDExtensionInterfaceDictionaryOperatorIndexConst dictionary_operator_index_const;

	GDExtensionInterfaceObjectMethodBindCall object_method_bind_call;
	GDExtensionInterfaceObjectMethodBindPtrcall object_method_bind_ptrcall;
	GDExtensionInterfaceObjectDestroy object_destroy;
	GDExtensionInterfaceGlobalGetSingleton global_get_singleton;
	GDExtensionInterfaceObjectGetInstanceBinding object_get_instance_binding;
	GDExtensionInterfaceObjectSetInstanceBinding object_set_instance_binding;
	GDExtensionInterfaceObjectSetInstance object_set_instance;
	GDExtensionInterfaceObjectCastTo object_cast_to;
	GDExtensionInterfaceObjectGetInstanceFromId object_get_instance_from_id;
	GDExtensionInterfaceObjectGetInstanceId object_get_instance_id;

	GDExtensionInterfaceRefGetObject ref_get_object;
	GDExtensionInterfaceRefSetObject ref_set_object;

	GDExtensionInterfaceScriptInstanceCreate script_instance_create;

	GDExtensionInterfaceClassdbConstructObject classdb_construct_object;
	GDExtensionInterfaceClassdbGetMethodBind classdb_get_method_bind;
	GDExtensionInterfaceClassdbGetClassTag classdb_get_class_tag;

	GDExtensionInterfaceClassdbRegisterExtensionClass classdb_register_extension_class;
	GDExtensionInterfaceClassdbRegisterExtensionClassMethod classdb_register_extension_class_method;
	GDExtensionInterfaceClassdbRegisterExtensionClassIntegerConstant classdb_register_extension_class_integer_constant;
	GDExtensionInterfaceClassdbRegisterExtensionClassProperty classdb_register_extension_class_property;
	GDExtensionInterfaceClassdbRegisterExtensionClassPropertyGroup classdb_register_extension_class_property_group;
	GDExtensionInterfaceClassdbRegisterExtensionClassPropertySubgroup classdb_register_extension_class_property_subgroup;
	GDExtensionInterfaceClassdbRegisterExtensionClassSignal classdb_register_extension_class_signal;
	GDExtensionInterfaceClassdbUnregisterExtensionClass classdb_unregister_extension_class;

	GDExtensionInterfaceGetLibraryPath get_library_path;

} LegacyGDExtensionInterface;

static LegacyGDExtensionInterface *legacy_gdextension_interface = nullptr;

#define SETUP_LEGACY_FUNC(m_name) *((GDExtensionInterfaceFunctionPtr *)legacy_gdextension_interface->##m_name) = GDExtension::get_interface_function(#m_name)

static void *gdextension_get_legacy_interface() {
	if (legacy_gdextension_interface != nullptr) {
		return legacy_gdextension_interface;
	}

	legacy_gdextension_interface = memnew(LegacyGDExtensionInterface);

	GDExtensionGodotVersion godot_version;
	gdextension_get_godot_version(&godot_version);
	legacy_gdextension_interface->version_major = godot_version.major;
	legacy_gdextension_interface->version_minor = godot_version.minor;
	legacy_gdextension_interface->version_patch = godot_version.patch;
	legacy_gdextension_interface->version_string = godot_version.string;

	SETUP_LEGACY_FUNC(mem_alloc);
	SETUP_LEGACY_FUNC(mem_realloc);
	SETUP_LEGACY_FUNC(mem_free);
	SETUP_LEGACY_FUNC(print_error);
	SETUP_LEGACY_FUNC(print_error_with_message);
	SETUP_LEGACY_FUNC(print_warning);
	SETUP_LEGACY_FUNC(print_warning_with_message);
	SETUP_LEGACY_FUNC(print_script_error);
	SETUP_LEGACY_FUNC(print_script_error_with_message);
	SETUP_LEGACY_FUNC(get_native_struct_size);
	SETUP_LEGACY_FUNC(variant_new_copy);
	SETUP_LEGACY_FUNC(variant_new_nil);
	SETUP_LEGACY_FUNC(variant_destroy);
	SETUP_LEGACY_FUNC(variant_call);
	SETUP_LEGACY_FUNC(variant_call_static);
	SETUP_LEGACY_FUNC(variant_evaluate);
	SETUP_LEGACY_FUNC(variant_set);
	SETUP_LEGACY_FUNC(variant_set_named);
	SETUP_LEGACY_FUNC(variant_set_keyed);
	SETUP_LEGACY_FUNC(variant_set_indexed);
	SETUP_LEGACY_FUNC(variant_get);
	SETUP_LEGACY_FUNC(variant_get_named);
	SETUP_LEGACY_FUNC(variant_get_keyed);
	SETUP_LEGACY_FUNC(variant_get_indexed);
	SETUP_LEGACY_FUNC(variant_iter_init);
	SETUP_LEGACY_FUNC(variant_iter_next);
	SETUP_LEGACY_FUNC(variant_iter_get);
	SETUP_LEGACY_FUNC(variant_hash);
	SETUP_LEGACY_FUNC(variant_recursive_hash);
	SETUP_LEGACY_FUNC(variant_hash_compare);
	SETUP_LEGACY_FUNC(variant_booleanize);
	SETUP_LEGACY_FUNC(variant_duplicate);
	SETUP_LEGACY_FUNC(variant_stringify);
	SETUP_LEGACY_FUNC(variant_get_type);
	SETUP_LEGACY_FUNC(variant_has_method);
	SETUP_LEGACY_FUNC(variant_has_member);
	SETUP_LEGACY_FUNC(variant_has_key);
	SETUP_LEGACY_FUNC(variant_get_type_name);
	SETUP_LEGACY_FUNC(variant_can_convert);
	SETUP_LEGACY_FUNC(variant_can_convert_strict);
	SETUP_LEGACY_FUNC(get_variant_from_type_constructor);
	SETUP_LEGACY_FUNC(get_variant_to_type_constructor);
	SETUP_LEGACY_FUNC(variant_get_ptr_operator_evaluator);
	SETUP_LEGACY_FUNC(variant_get_ptr_builtin_method);
	SETUP_LEGACY_FUNC(variant_get_ptr_constructor);
	SETUP_LEGACY_FUNC(variant_get_ptr_destructor);
	SETUP_LEGACY_FUNC(variant_construct);
	SETUP_LEGACY_FUNC(variant_get_ptr_setter);
	SETUP_LEGACY_FUNC(variant_get_ptr_getter);
	SETUP_LEGACY_FUNC(variant_get_ptr_indexed_setter);
	SETUP_LEGACY_FUNC(variant_get_ptr_indexed_getter);
	SETUP_LEGACY_FUNC(variant_get_ptr_keyed_setter);
	SETUP_LEGACY_FUNC(variant_get_ptr_keyed_getter);
	SETUP_LEGACY_FUNC(variant_get_ptr_keyed_checker);
	SETUP_LEGACY_FUNC(variant_get_constant_value);
	SETUP_LEGACY_FUNC(variant_get_ptr_utility_function);
	SETUP_LEGACY_FUNC(string_new_with_latin1_chars);
	SETUP_LEGACY_FUNC(string_new_with_utf8_chars);
	SETUP_LEGACY_FUNC(string_new_with_utf16_chars);
	SETUP_LEGACY_FUNC(string_new_with_utf32_chars);
	SETUP_LEGACY_FUNC(string_new_with_wide_chars);
	SETUP_LEGACY_FUNC(string_new_with_latin1_chars_and_len);
	SETUP_LEGACY_FUNC(string_new_with_utf8_chars_and_len);
	SETUP_LEGACY_FUNC(string_new_with_utf16_chars_and_len);
	SETUP_LEGACY_FUNC(string_new_with_utf32_chars_and_len);
	SETUP_LEGACY_FUNC(string_new_with_wide_chars_and_len);
	SETUP_LEGACY_FUNC(string_to_latin1_chars);
	SETUP_LEGACY_FUNC(string_to_utf8_chars);
	SETUP_LEGACY_FUNC(string_to_utf16_chars);
	SETUP_LEGACY_FUNC(string_to_utf32_chars);
	SETUP_LEGACY_FUNC(string_to_wide_chars);
	SETUP_LEGACY_FUNC(string_operator_index);
	SETUP_LEGACY_FUNC(string_operator_index_const);
	SETUP_LEGACY_FUNC(string_operator_plus_eq_string);
	SETUP_LEGACY_FUNC(string_operator_plus_eq_char);
	SETUP_LEGACY_FUNC(string_operator_plus_eq_cstr);
	SETUP_LEGACY_FUNC(string_operator_plus_eq_wcstr);
	SETUP_LEGACY_FUNC(string_operator_plus_eq_c32str);
	SETUP_LEGACY_FUNC(xml_parser_open_buffer);
	SETUP_LEGACY_FUNC(file_access_store_buffer);
	SETUP_LEGACY_FUNC(file_access_get_buffer);
	SETUP_LEGACY_FUNC(worker_thread_pool_add_native_group_task);
	SETUP_LEGACY_FUNC(worker_thread_pool_add_native_task);
	SETUP_LEGACY_FUNC(packed_byte_array_operator_index);
	SETUP_LEGACY_FUNC(packed_byte_array_operator_index_const);
	SETUP_LEGACY_FUNC(packed_color_array_operator_index);
	SETUP_LEGACY_FUNC(packed_color_array_operator_index_const);
	SETUP_LEGACY_FUNC(packed_float32_array_operator_index);
	SETUP_LEGACY_FUNC(packed_float32_array_operator_index_const);
	SETUP_LEGACY_FUNC(packed_float64_array_operator_index);
	SETUP_LEGACY_FUNC(packed_float64_array_operator_index_const);
	SETUP_LEGACY_FUNC(packed_int32_array_operator_index);
	SETUP_LEGACY_FUNC(packed_int32_array_operator_index_const);
	SETUP_LEGACY_FUNC(packed_int64_array_operator_index);
	SETUP_LEGACY_FUNC(packed_int64_array_operator_index_const);
	SETUP_LEGACY_FUNC(packed_string_array_operator_index);
	SETUP_LEGACY_FUNC(packed_string_array_operator_index_const);
	SETUP_LEGACY_FUNC(packed_vector2_array_operator_index);
	SETUP_LEGACY_FUNC(packed_vector2_array_operator_index_const);
	SETUP_LEGACY_FUNC(packed_vector3_array_operator_index);
	SETUP_LEGACY_FUNC(packed_vector3_array_operator_index_const);
	SETUP_LEGACY_FUNC(array_operator_index);
	SETUP_LEGACY_FUNC(array_operator_index_const);
	SETUP_LEGACY_FUNC(array_ref);
	SETUP_LEGACY_FUNC(array_set_typed);
	SETUP_LEGACY_FUNC(dictionary_operator_index);
	SETUP_LEGACY_FUNC(dictionary_operator_index_const);
	SETUP_LEGACY_FUNC(object_method_bind_call);
	SETUP_LEGACY_FUNC(object_method_bind_ptrcall);
	SETUP_LEGACY_FUNC(object_destroy);
	SETUP_LEGACY_FUNC(global_get_singleton);
	SETUP_LEGACY_FUNC(object_get_instance_binding);
	SETUP_LEGACY_FUNC(object_set_instance_binding);
	SETUP_LEGACY_FUNC(object_set_instance);
	SETUP_LEGACY_FUNC(object_cast_to);
	SETUP_LEGACY_FUNC(object_get_instance_from_id);
	SETUP_LEGACY_FUNC(object_get_instance_id);
	SETUP_LEGACY_FUNC(ref_get_object);
	SETUP_LEGACY_FUNC(ref_set_object);
	SETUP_LEGACY_FUNC(script_instance_create);
	SETUP_LEGACY_FUNC(classdb_construct_object);
	SETUP_LEGACY_FUNC(classdb_get_method_bind);
	SETUP_LEGACY_FUNC(classdb_get_class_tag);
	SETUP_LEGACY_FUNC(classdb_register_extension_class);
	SETUP_LEGACY_FUNC(classdb_register_extension_class_method);
	SETUP_LEGACY_FUNC(classdb_register_extension_class_integer_constant);
	SETUP_LEGACY_FUNC(classdb_register_extension_class_property);
	SETUP_LEGACY_FUNC(classdb_register_extension_class_property_group);
	SETUP_LEGACY_FUNC(classdb_register_extension_class_property_subgroup);
	SETUP_LEGACY_FUNC(classdb_register_extension_class_signal);
	SETUP_LEGACY_FUNC(classdb_unregister_extension_class);
	SETUP_LEGACY_FUNC(get_library_path);

	return legacy_gdextension_interface;
}

#undef SETUP_LEGACY_FUNC
