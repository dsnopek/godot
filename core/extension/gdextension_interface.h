/**************************************************************************/
/*  gdextension_interface.h                                               */
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

#ifndef GDEXTENSION_INTERFACE_H
#define GDEXTENSION_INTERFACE_H

/* This is a C class header, you can copy it and use it directly in your own binders.
 * Together with the JSON file, you should be able to generate any binder.
 */

#include <stddef.h>
#include <stdint.h>

#ifndef __cplusplus
typedef uint32_t char32_t;
typedef uint16_t char16_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* VARIANT TYPES */

typedef enum {
	GDEXTENSION_VARIANT_TYPE_NIL,

	/*  atomic types */
	GDEXTENSION_VARIANT_TYPE_BOOL,
	GDEXTENSION_VARIANT_TYPE_INT,
	GDEXTENSION_VARIANT_TYPE_FLOAT,
	GDEXTENSION_VARIANT_TYPE_STRING,

	/* math types */
	GDEXTENSION_VARIANT_TYPE_VECTOR2,
	GDEXTENSION_VARIANT_TYPE_VECTOR2I,
	GDEXTENSION_VARIANT_TYPE_RECT2,
	GDEXTENSION_VARIANT_TYPE_RECT2I,
	GDEXTENSION_VARIANT_TYPE_VECTOR3,
	GDEXTENSION_VARIANT_TYPE_VECTOR3I,
	GDEXTENSION_VARIANT_TYPE_TRANSFORM2D,
	GDEXTENSION_VARIANT_TYPE_VECTOR4,
	GDEXTENSION_VARIANT_TYPE_VECTOR4I,
	GDEXTENSION_VARIANT_TYPE_PLANE,
	GDEXTENSION_VARIANT_TYPE_QUATERNION,
	GDEXTENSION_VARIANT_TYPE_AABB,
	GDEXTENSION_VARIANT_TYPE_BASIS,
	GDEXTENSION_VARIANT_TYPE_TRANSFORM3D,
	GDEXTENSION_VARIANT_TYPE_PROJECTION,

	/* misc types */
	GDEXTENSION_VARIANT_TYPE_COLOR,
	GDEXTENSION_VARIANT_TYPE_STRING_NAME,
	GDEXTENSION_VARIANT_TYPE_NODE_PATH,
	GDEXTENSION_VARIANT_TYPE_RID,
	GDEXTENSION_VARIANT_TYPE_OBJECT,
	GDEXTENSION_VARIANT_TYPE_CALLABLE,
	GDEXTENSION_VARIANT_TYPE_SIGNAL,
	GDEXTENSION_VARIANT_TYPE_DICTIONARY,
	GDEXTENSION_VARIANT_TYPE_ARRAY,

	/* typed arrays */
	GDEXTENSION_VARIANT_TYPE_PACKED_BYTE_ARRAY,
	GDEXTENSION_VARIANT_TYPE_PACKED_INT32_ARRAY,
	GDEXTENSION_VARIANT_TYPE_PACKED_INT64_ARRAY,
	GDEXTENSION_VARIANT_TYPE_PACKED_FLOAT32_ARRAY,
	GDEXTENSION_VARIANT_TYPE_PACKED_FLOAT64_ARRAY,
	GDEXTENSION_VARIANT_TYPE_PACKED_STRING_ARRAY,
	GDEXTENSION_VARIANT_TYPE_PACKED_VECTOR2_ARRAY,
	GDEXTENSION_VARIANT_TYPE_PACKED_VECTOR3_ARRAY,
	GDEXTENSION_VARIANT_TYPE_PACKED_COLOR_ARRAY,

	GDEXTENSION_VARIANT_TYPE_VARIANT_MAX
} GDExtensionVariantType;

typedef enum {
	/* comparison */
	GDEXTENSION_VARIANT_OP_EQUAL,
	GDEXTENSION_VARIANT_OP_NOT_EQUAL,
	GDEXTENSION_VARIANT_OP_LESS,
	GDEXTENSION_VARIANT_OP_LESS_EQUAL,
	GDEXTENSION_VARIANT_OP_GREATER,
	GDEXTENSION_VARIANT_OP_GREATER_EQUAL,

	/* mathematic */
	GDEXTENSION_VARIANT_OP_ADD,
	GDEXTENSION_VARIANT_OP_SUBTRACT,
	GDEXTENSION_VARIANT_OP_MULTIPLY,
	GDEXTENSION_VARIANT_OP_DIVIDE,
	GDEXTENSION_VARIANT_OP_NEGATE,
	GDEXTENSION_VARIANT_OP_POSITIVE,
	GDEXTENSION_VARIANT_OP_MODULE,
	GDEXTENSION_VARIANT_OP_POWER,

	/* bitwise */
	GDEXTENSION_VARIANT_OP_SHIFT_LEFT,
	GDEXTENSION_VARIANT_OP_SHIFT_RIGHT,
	GDEXTENSION_VARIANT_OP_BIT_AND,
	GDEXTENSION_VARIANT_OP_BIT_OR,
	GDEXTENSION_VARIANT_OP_BIT_XOR,
	GDEXTENSION_VARIANT_OP_BIT_NEGATE,

	/* logic */
	GDEXTENSION_VARIANT_OP_AND,
	GDEXTENSION_VARIANT_OP_OR,
	GDEXTENSION_VARIANT_OP_XOR,
	GDEXTENSION_VARIANT_OP_NOT,

	/* containment */
	GDEXTENSION_VARIANT_OP_IN,
	GDEXTENSION_VARIANT_OP_MAX

} GDExtensionVariantOperator;

typedef void *GDExtensionVariantPtr;
typedef const void *GDExtensionConstVariantPtr;
typedef void *GDExtensionStringNamePtr;
typedef const void *GDExtensionConstStringNamePtr;
typedef void *GDExtensionStringPtr;
typedef const void *GDExtensionConstStringPtr;
typedef void *GDExtensionObjectPtr;
typedef const void *GDExtensionConstObjectPtr;
typedef void *GDExtensionTypePtr;
typedef const void *GDExtensionConstTypePtr;
typedef const void *GDExtensionMethodBindPtr;
typedef int64_t GDExtensionInt;
typedef uint8_t GDExtensionBool;
typedef uint64_t GDObjectInstanceID;
typedef void *GDExtensionRefPtr;
typedef const void *GDExtensionConstRefPtr;

/* VARIANT DATA I/O */

typedef enum {
	GDEXTENSION_CALL_OK,
	GDEXTENSION_CALL_ERROR_INVALID_METHOD,
	GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT, // Expected a different variant type.
	GDEXTENSION_CALL_ERROR_TOO_MANY_ARGUMENTS, // Expected lower number of arguments.
	GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS, // Expected higher number of arguments.
	GDEXTENSION_CALL_ERROR_INSTANCE_IS_NULL,
	GDEXTENSION_CALL_ERROR_METHOD_NOT_CONST, // Used for const call.
} GDExtensionCallErrorType;

typedef struct {
	GDExtensionCallErrorType error;
	int32_t argument;
	int32_t expected;
} GDExtensionCallError;

typedef void (*GDExtensionVariantFromTypeConstructorFunc)(GDExtensionVariantPtr, GDExtensionTypePtr);
typedef void (*GDExtensionTypeFromVariantConstructorFunc)(GDExtensionTypePtr, GDExtensionVariantPtr);
typedef void (*GDExtensionPtrOperatorEvaluator)(GDExtensionConstTypePtr p_left, GDExtensionConstTypePtr p_right, GDExtensionTypePtr r_result);
typedef void (*GDExtensionPtrBuiltInMethod)(GDExtensionTypePtr p_base, const GDExtensionConstTypePtr *p_args, GDExtensionTypePtr r_return, int p_argument_count);
typedef void (*GDExtensionPtrConstructor)(GDExtensionTypePtr p_base, const GDExtensionConstTypePtr *p_args);
typedef void (*GDExtensionPtrDestructor)(GDExtensionTypePtr p_base);
typedef void (*GDExtensionPtrSetter)(GDExtensionTypePtr p_base, GDExtensionConstTypePtr p_value);
typedef void (*GDExtensionPtrGetter)(GDExtensionConstTypePtr p_base, GDExtensionTypePtr r_value);
typedef void (*GDExtensionPtrIndexedSetter)(GDExtensionTypePtr p_base, GDExtensionInt p_index, GDExtensionConstTypePtr p_value);
typedef void (*GDExtensionPtrIndexedGetter)(GDExtensionConstTypePtr p_base, GDExtensionInt p_index, GDExtensionTypePtr r_value);
typedef void (*GDExtensionPtrKeyedSetter)(GDExtensionTypePtr p_base, GDExtensionConstTypePtr p_key, GDExtensionConstTypePtr p_value);
typedef void (*GDExtensionPtrKeyedGetter)(GDExtensionConstTypePtr p_base, GDExtensionConstTypePtr p_key, GDExtensionTypePtr r_value);
typedef uint32_t (*GDExtensionPtrKeyedChecker)(GDExtensionConstVariantPtr p_base, GDExtensionConstVariantPtr p_key);
typedef void (*GDExtensionPtrUtilityFunction)(GDExtensionTypePtr r_return, const GDExtensionConstTypePtr *p_args, int p_argument_count);

typedef GDExtensionObjectPtr (*GDExtensionClassConstructor)();

typedef void *(*GDExtensionInstanceBindingCreateCallback)(void *p_token, void *p_instance);
typedef void (*GDExtensionInstanceBindingFreeCallback)(void *p_token, void *p_instance, void *p_binding);
typedef GDExtensionBool (*GDExtensionInstanceBindingReferenceCallback)(void *p_token, void *p_binding, GDExtensionBool p_reference);

typedef struct {
	GDExtensionInstanceBindingCreateCallback create_callback;
	GDExtensionInstanceBindingFreeCallback free_callback;
	GDExtensionInstanceBindingReferenceCallback reference_callback;
} GDExtensionInstanceBindingCallbacks;

/* EXTENSION CLASSES */

typedef void *GDExtensionClassInstancePtr;

typedef GDExtensionBool (*GDExtensionClassSet)(GDExtensionClassInstancePtr p_instance, GDExtensionConstStringNamePtr p_name, GDExtensionConstVariantPtr p_value);
typedef GDExtensionBool (*GDExtensionClassGet)(GDExtensionClassInstancePtr p_instance, GDExtensionConstStringNamePtr p_name, GDExtensionVariantPtr r_ret);
typedef uint64_t (*GDExtensionClassGetRID)(GDExtensionClassInstancePtr p_instance);

typedef struct {
	GDExtensionVariantType type;
	GDExtensionStringNamePtr name;
	GDExtensionStringNamePtr class_name;
	uint32_t hint; // Bitfield of `PropertyHint` (defined in `extension_api.json`).
	GDExtensionStringPtr hint_string;
	uint32_t usage; // Bitfield of `PropertyUsageFlags` (defined in `extension_api.json`).
} GDExtensionPropertyInfo;

typedef struct {
	GDExtensionStringNamePtr name;
	GDExtensionPropertyInfo return_value;
	uint32_t flags; // Bitfield of `GDExtensionClassMethodFlags`.
	int32_t id;

	/* Arguments: `default_arguments` is an array of size `argument_count`. */
	uint32_t argument_count;
	GDExtensionPropertyInfo *arguments;

	/* Default arguments: `default_arguments` is an array of size `default_argument_count`. */
	uint32_t default_argument_count;
	GDExtensionVariantPtr *default_arguments;
} GDExtensionMethodInfo;

typedef const GDExtensionPropertyInfo *(*GDExtensionClassGetPropertyList)(GDExtensionClassInstancePtr p_instance, uint32_t *r_count);
typedef void (*GDExtensionClassFreePropertyList)(GDExtensionClassInstancePtr p_instance, const GDExtensionPropertyInfo *p_list);
typedef GDExtensionBool (*GDExtensionClassPropertyCanRevert)(GDExtensionClassInstancePtr p_instance, GDExtensionConstStringNamePtr p_name);
typedef GDExtensionBool (*GDExtensionClassPropertyGetRevert)(GDExtensionClassInstancePtr p_instance, GDExtensionConstStringNamePtr p_name, GDExtensionVariantPtr r_ret);
typedef void (*GDExtensionClassNotification)(GDExtensionClassInstancePtr p_instance, int32_t p_what);
typedef void (*GDExtensionClassToString)(GDExtensionClassInstancePtr p_instance, GDExtensionBool *r_is_valid, GDExtensionStringPtr p_out);
typedef void (*GDExtensionClassReference)(GDExtensionClassInstancePtr p_instance);
typedef void (*GDExtensionClassUnreference)(GDExtensionClassInstancePtr p_instance);
typedef void (*GDExtensionClassCallVirtual)(GDExtensionClassInstancePtr p_instance, const GDExtensionConstTypePtr *p_args, GDExtensionTypePtr r_ret);
typedef GDExtensionObjectPtr (*GDExtensionClassCreateInstance)(void *p_userdata);
typedef void (*GDExtensionClassFreeInstance)(void *p_userdata, GDExtensionClassInstancePtr p_instance);
typedef GDExtensionClassCallVirtual (*GDExtensionClassGetVirtual)(void *p_userdata, GDExtensionConstStringNamePtr p_name);

typedef struct {
	GDExtensionBool is_virtual;
	GDExtensionBool is_abstract;
	GDExtensionClassSet set_func;
	GDExtensionClassGet get_func;
	GDExtensionClassGetPropertyList get_property_list_func;
	GDExtensionClassFreePropertyList free_property_list_func;
	GDExtensionClassPropertyCanRevert property_can_revert_func;
	GDExtensionClassPropertyGetRevert property_get_revert_func;
	GDExtensionClassNotification notification_func;
	GDExtensionClassToString to_string_func;
	GDExtensionClassReference reference_func;
	GDExtensionClassUnreference unreference_func;
	GDExtensionClassCreateInstance create_instance_func; // (Default) constructor; mandatory. If the class is not instantiable, consider making it virtual or abstract.
	GDExtensionClassFreeInstance free_instance_func; // Destructor; mandatory.
	GDExtensionClassGetVirtual get_virtual_func; // Queries a virtual function by name and returns a callback to invoke the requested virtual function.
	GDExtensionClassGetRID get_rid_func;
	void *class_userdata; // Per-class user data, later accessible in instance bindings.
} GDExtensionClassCreationInfo;

typedef void *GDExtensionClassLibraryPtr;

/* Method */

typedef enum {
	GDEXTENSION_METHOD_FLAG_NORMAL = 1,
	GDEXTENSION_METHOD_FLAG_EDITOR = 2,
	GDEXTENSION_METHOD_FLAG_CONST = 4,
	GDEXTENSION_METHOD_FLAG_VIRTUAL = 8,
	GDEXTENSION_METHOD_FLAG_VARARG = 16,
	GDEXTENSION_METHOD_FLAG_STATIC = 32,
	GDEXTENSION_METHOD_FLAGS_DEFAULT = GDEXTENSION_METHOD_FLAG_NORMAL,
} GDExtensionClassMethodFlags;

typedef enum {
	GDEXTENSION_METHOD_ARGUMENT_METADATA_NONE,
	GDEXTENSION_METHOD_ARGUMENT_METADATA_INT_IS_INT8,
	GDEXTENSION_METHOD_ARGUMENT_METADATA_INT_IS_INT16,
	GDEXTENSION_METHOD_ARGUMENT_METADATA_INT_IS_INT32,
	GDEXTENSION_METHOD_ARGUMENT_METADATA_INT_IS_INT64,
	GDEXTENSION_METHOD_ARGUMENT_METADATA_INT_IS_UINT8,
	GDEXTENSION_METHOD_ARGUMENT_METADATA_INT_IS_UINT16,
	GDEXTENSION_METHOD_ARGUMENT_METADATA_INT_IS_UINT32,
	GDEXTENSION_METHOD_ARGUMENT_METADATA_INT_IS_UINT64,
	GDEXTENSION_METHOD_ARGUMENT_METADATA_REAL_IS_FLOAT,
	GDEXTENSION_METHOD_ARGUMENT_METADATA_REAL_IS_DOUBLE
} GDExtensionClassMethodArgumentMetadata;

typedef void (*GDExtensionClassMethodCall)(void *method_userdata, GDExtensionClassInstancePtr p_instance, const GDExtensionConstVariantPtr *p_args, GDExtensionInt p_argument_count, GDExtensionVariantPtr r_return, GDExtensionCallError *r_error);
typedef void (*GDExtensionClassMethodPtrCall)(void *method_userdata, GDExtensionClassInstancePtr p_instance, const GDExtensionConstTypePtr *p_args, GDExtensionTypePtr r_ret);

typedef struct {
	GDExtensionStringNamePtr name;
	void *method_userdata;
	GDExtensionClassMethodCall call_func;
	GDExtensionClassMethodPtrCall ptrcall_func;
	uint32_t method_flags; // Bitfield of `GDExtensionClassMethodFlags`.

	/* If `has_return_value` is false, `return_value_info` and `return_value_metadata` are ignored. */
	GDExtensionBool has_return_value;
	GDExtensionPropertyInfo *return_value_info;
	GDExtensionClassMethodArgumentMetadata return_value_metadata;

	/* Arguments: `arguments_info` and `arguments_metadata` are array of size `argument_count`.
	 * Name and hint information for the argument can be omitted in release builds. Class name should always be present if it applies.
	 */
	uint32_t argument_count;
	GDExtensionPropertyInfo *arguments_info;
	GDExtensionClassMethodArgumentMetadata *arguments_metadata;

	/* Default arguments: `default_arguments` is an array of size `default_argument_count`. */
	uint32_t default_argument_count;
	GDExtensionVariantPtr *default_arguments;
} GDExtensionClassMethodInfo;

/* SCRIPT INSTANCE EXTENSION */

typedef void *GDExtensionScriptInstanceDataPtr; // Pointer to custom ScriptInstance native implementation.

typedef GDExtensionBool (*GDExtensionScriptInstanceSet)(GDExtensionScriptInstanceDataPtr p_instance, GDExtensionConstStringNamePtr p_name, GDExtensionConstVariantPtr p_value);
typedef GDExtensionBool (*GDExtensionScriptInstanceGet)(GDExtensionScriptInstanceDataPtr p_instance, GDExtensionConstStringNamePtr p_name, GDExtensionVariantPtr r_ret);
typedef const GDExtensionPropertyInfo *(*GDExtensionScriptInstanceGetPropertyList)(GDExtensionScriptInstanceDataPtr p_instance, uint32_t *r_count);
typedef void (*GDExtensionScriptInstanceFreePropertyList)(GDExtensionScriptInstanceDataPtr p_instance, const GDExtensionPropertyInfo *p_list);
typedef GDExtensionVariantType (*GDExtensionScriptInstanceGetPropertyType)(GDExtensionScriptInstanceDataPtr p_instance, GDExtensionConstStringNamePtr p_name, GDExtensionBool *r_is_valid);

typedef GDExtensionBool (*GDExtensionScriptInstancePropertyCanRevert)(GDExtensionScriptInstanceDataPtr p_instance, GDExtensionConstStringNamePtr p_name);
typedef GDExtensionBool (*GDExtensionScriptInstancePropertyGetRevert)(GDExtensionScriptInstanceDataPtr p_instance, GDExtensionConstStringNamePtr p_name, GDExtensionVariantPtr r_ret);

typedef GDExtensionObjectPtr (*GDExtensionScriptInstanceGetOwner)(GDExtensionScriptInstanceDataPtr p_instance);
typedef void (*GDExtensionScriptInstancePropertyStateAdd)(GDExtensionConstStringNamePtr p_name, GDExtensionConstVariantPtr p_value, void *p_userdata);
typedef void (*GDExtensionScriptInstanceGetPropertyState)(GDExtensionScriptInstanceDataPtr p_instance, GDExtensionScriptInstancePropertyStateAdd p_add_func, void *p_userdata);

typedef const GDExtensionMethodInfo *(*GDExtensionScriptInstanceGetMethodList)(GDExtensionScriptInstanceDataPtr p_instance, uint32_t *r_count);
typedef void (*GDExtensionScriptInstanceFreeMethodList)(GDExtensionScriptInstanceDataPtr p_instance, const GDExtensionMethodInfo *p_list);

typedef GDExtensionBool (*GDExtensionScriptInstanceHasMethod)(GDExtensionScriptInstanceDataPtr p_instance, GDExtensionConstStringNamePtr p_name);

typedef void (*GDExtensionScriptInstanceCall)(GDExtensionScriptInstanceDataPtr p_self, GDExtensionConstStringNamePtr p_method, const GDExtensionConstVariantPtr *p_args, GDExtensionInt p_argument_count, GDExtensionVariantPtr r_return, GDExtensionCallError *r_error);
typedef void (*GDExtensionScriptInstanceNotification)(GDExtensionScriptInstanceDataPtr p_instance, int32_t p_what);
typedef void (*GDExtensionScriptInstanceToString)(GDExtensionScriptInstanceDataPtr p_instance, GDExtensionBool *r_is_valid, GDExtensionStringPtr r_out);

typedef void (*GDExtensionScriptInstanceRefCountIncremented)(GDExtensionScriptInstanceDataPtr p_instance);
typedef GDExtensionBool (*GDExtensionScriptInstanceRefCountDecremented)(GDExtensionScriptInstanceDataPtr p_instance);

typedef GDExtensionObjectPtr (*GDExtensionScriptInstanceGetScript)(GDExtensionScriptInstanceDataPtr p_instance);
typedef GDExtensionBool (*GDExtensionScriptInstanceIsPlaceholder)(GDExtensionScriptInstanceDataPtr p_instance);

typedef void *GDExtensionScriptLanguagePtr;

typedef GDExtensionScriptLanguagePtr (*GDExtensionScriptInstanceGetLanguage)(GDExtensionScriptInstanceDataPtr p_instance);

typedef void (*GDExtensionScriptInstanceFree)(GDExtensionScriptInstanceDataPtr p_instance);

typedef void *GDExtensionScriptInstancePtr; // Pointer to ScriptInstance.

typedef struct {
	GDExtensionScriptInstanceSet set_func;
	GDExtensionScriptInstanceGet get_func;
	GDExtensionScriptInstanceGetPropertyList get_property_list_func;
	GDExtensionScriptInstanceFreePropertyList free_property_list_func;

	GDExtensionScriptInstancePropertyCanRevert property_can_revert_func;
	GDExtensionScriptInstancePropertyGetRevert property_get_revert_func;

	GDExtensionScriptInstanceGetOwner get_owner_func;
	GDExtensionScriptInstanceGetPropertyState get_property_state_func;

	GDExtensionScriptInstanceGetMethodList get_method_list_func;
	GDExtensionScriptInstanceFreeMethodList free_method_list_func;
	GDExtensionScriptInstanceGetPropertyType get_property_type_func;

	GDExtensionScriptInstanceHasMethod has_method_func;

	GDExtensionScriptInstanceCall call_func;
	GDExtensionScriptInstanceNotification notification_func;

	GDExtensionScriptInstanceToString to_string_func;

	GDExtensionScriptInstanceRefCountIncremented refcount_incremented_func;
	GDExtensionScriptInstanceRefCountDecremented refcount_decremented_func;

	GDExtensionScriptInstanceGetScript get_script_func;

	GDExtensionScriptInstanceIsPlaceholder is_placeholder_func;

	GDExtensionScriptInstanceSet set_fallback_func;
	GDExtensionScriptInstanceGet get_fallback_func;

	GDExtensionScriptInstanceGetLanguage get_language_func;

	GDExtensionScriptInstanceFree free_func;

} GDExtensionScriptInstanceInfo;

/* INTERFACE */

typedef struct {
	uint32_t major;
	uint32_t minor;
	uint32_t patch;
	const char *string;
} GDExtensionGodotVersion;

typedef void (*GDExtensionInterfaceFunctionPtr)();

typedef GDExtensionInterfaceFunctionPtr (*GDExtensionInterfaceGetProcAddress)(const char *p_function_name);
typedef void (*GDExtensionInterfaceGetGodotVersion)(GDExtensionGodotVersion *r_godot_version);

/* INTERFACE: Memory */

typedef void *(*GDExtensionInterfaceMemAlloc)(size_t p_bytes);
typedef void *(*GDExtensionInterfaceMemRealloc)(void *p_ptr, size_t p_bytes);
typedef void (*GDExtensionInterfaceMemFree)(void *p_ptr);

/* INTERFACE: Godot Core */

typedef void (*GDExtensionInterfacePrintError)(const char *p_description, const char *p_function, const char *p_file, int32_t p_line, GDExtensionBool p_editor_notify);
typedef void (*GDExtensionInterfacePrintErrorWithMessage)(const char *p_description, const char *p_message, const char *p_function, const char *p_file, int32_t p_line, GDExtensionBool p_editor_notify);
typedef void (*GDExtensionInterfacePrintWarning)(const char *p_description, const char *p_function, const char *p_file, int32_t p_line, GDExtensionBool p_editor_notify);
typedef void (*GDExtensionInterfacePrintWarningWithMessage)(const char *p_description, const char *p_message, const char *p_function, const char *p_file, int32_t p_line, GDExtensionBool p_editor_notify);
typedef void (*GDExtensionInterfacePrintScriptError)(const char *p_description, const char *p_function, const char *p_file, int32_t p_line, GDExtensionBool p_editor_notify);
typedef void (*GDExtensionInterfacePrintScriptErrorWithMessage)(const char *p_description, const char *p_message, const char *p_function, const char *p_file, int32_t p_line, GDExtensionBool p_editor_notify);

typedef uint64_t (*GDExtensionInterfaceGetNativeStructSize)(GDExtensionConstStringNamePtr p_name);

/* INTERFACE: Variant */

typedef void (*GDExtensionInterfaceVariantNewCopy)(GDExtensionVariantPtr r_dest, GDExtensionConstVariantPtr p_src);
typedef void (*GDExtensionInterfaceVariantNewNil)(GDExtensionVariantPtr r_dest);
typedef void (*GDExtensionInterfaceVariantDestroy)(GDExtensionVariantPtr p_self);

typedef void (*GDExtensionInterfaceVariantCall)(GDExtensionVariantPtr p_self, GDExtensionConstStringNamePtr p_method, const GDExtensionConstVariantPtr *p_args, GDExtensionInt p_argument_count, GDExtensionVariantPtr r_return, GDExtensionCallError *r_error);
typedef void (*GDExtensionInterfaceVariantCallStatic)(GDExtensionVariantType p_type, GDExtensionConstStringNamePtr p_method, const GDExtensionConstVariantPtr *p_args, GDExtensionInt p_argument_count, GDExtensionVariantPtr r_return, GDExtensionCallError *r_error);
typedef void (*GDExtensionInterfaceVariantEvaluate)(GDExtensionVariantOperator p_op, GDExtensionConstVariantPtr p_a, GDExtensionConstVariantPtr p_b, GDExtensionVariantPtr r_return, GDExtensionBool *r_valid);
typedef void (*GDExtensionInterfaceVariantSet)(GDExtensionVariantPtr p_self, GDExtensionConstVariantPtr p_key, GDExtensionConstVariantPtr p_value, GDExtensionBool *r_valid);
typedef void (*GDExtensionInterfaceVariantSetNamed)(GDExtensionVariantPtr p_self, GDExtensionConstStringNamePtr p_key, GDExtensionConstVariantPtr p_value, GDExtensionBool *r_valid);
typedef void (*GDExtensionInterfaceVariantSetKeyed)(GDExtensionVariantPtr p_self, GDExtensionConstVariantPtr p_key, GDExtensionConstVariantPtr p_value, GDExtensionBool *r_valid);
typedef void (*GDExtensionInterfaceVariantSetIndexed)(GDExtensionVariantPtr p_self, GDExtensionInt p_index, GDExtensionConstVariantPtr p_value, GDExtensionBool *r_valid, GDExtensionBool *r_oob);
typedef void (*GDExtensionInterfaceVariantGet)(GDExtensionConstVariantPtr p_self, GDExtensionConstVariantPtr p_key, GDExtensionVariantPtr r_ret, GDExtensionBool *r_valid);
typedef void (*GDExtensionInterfaceVariantGetNamed)(GDExtensionConstVariantPtr p_self, GDExtensionConstStringNamePtr p_key, GDExtensionVariantPtr r_ret, GDExtensionBool *r_valid);
typedef void (*GDExtensionInterfaceVariantGetKeyed)(GDExtensionConstVariantPtr p_self, GDExtensionConstVariantPtr p_key, GDExtensionVariantPtr r_ret, GDExtensionBool *r_valid);
typedef void (*GDExtensionInterfaceVariantGetIndexed)(GDExtensionConstVariantPtr p_self, GDExtensionInt p_index, GDExtensionVariantPtr r_ret, GDExtensionBool *r_valid, GDExtensionBool *r_oob);
typedef GDExtensionBool (*GDExtensionInterfaceVariantIterInit)(GDExtensionConstVariantPtr p_self, GDExtensionVariantPtr r_iter, GDExtensionBool *r_valid);
typedef GDExtensionBool (*GDExtensionInterfaceVariantIterNext)(GDExtensionConstVariantPtr p_self, GDExtensionVariantPtr r_iter, GDExtensionBool *r_valid);
typedef void (*GDExtensionInterfaceVariantIterGet)(GDExtensionConstVariantPtr p_self, GDExtensionVariantPtr r_iter, GDExtensionVariantPtr r_ret, GDExtensionBool *r_valid);
typedef GDExtensionInt (*GDExtensionInterfaceVariantHash)(GDExtensionConstVariantPtr p_self);
typedef GDExtensionInt (*GDExtensionInterfaceVariantRecursiveHash)(GDExtensionConstVariantPtr p_self, GDExtensionInt p_recursion_count);
typedef GDExtensionBool (*GDExtensionInterfaceVariantHashCompare)(GDExtensionConstVariantPtr p_self, GDExtensionConstVariantPtr p_other);
typedef GDExtensionBool (*GDExtensionInterfaceVariantBooleanize)(GDExtensionConstVariantPtr p_self);
typedef void (*GDExtensionInterfaceVariantDuplicate)(GDExtensionConstVariantPtr p_self, GDExtensionVariantPtr r_ret, GDExtensionBool p_deep);
typedef void (*GDExtensionInterfaceVariantStringify)(GDExtensionConstVariantPtr p_self, GDExtensionStringPtr r_ret);

typedef GDExtensionVariantType (*GDExtensionInterfaceVariantGetType)(GDExtensionConstVariantPtr p_self);
typedef GDExtensionBool (*GDExtensionInterfaceVariantHasMethod)(GDExtensionConstVariantPtr p_self, GDExtensionConstStringNamePtr p_method);
typedef GDExtensionBool (*GDExtensionInterfaceVariantHasMember)(GDExtensionVariantType p_type, GDExtensionConstStringNamePtr p_member);
typedef GDExtensionBool (*GDExtensionInterfaceVariantHasKey)(GDExtensionConstVariantPtr p_self, GDExtensionConstVariantPtr p_key, GDExtensionBool *r_valid);
typedef void (*GDExtensionInterfaceVariantGetTypeName)(GDExtensionVariantType p_type, GDExtensionStringPtr r_name);
typedef GDExtensionBool (*GDExtensionInterfaceVariantCanConvert)(GDExtensionVariantType p_from, GDExtensionVariantType p_to);
typedef GDExtensionBool (*GDExtensionInterfaceVariantCanConvertStrict)(GDExtensionVariantType p_from, GDExtensionVariantType p_to);

typedef GDExtensionVariantFromTypeConstructorFunc (*GDExtensionInterfaceGetVariantFromTypeConstructor)(GDExtensionVariantType p_type);
typedef GDExtensionTypeFromVariantConstructorFunc (*GDExtensionInterfaceGetVariantToTypeConstructor)(GDExtensionVariantType p_type);
typedef GDExtensionPtrOperatorEvaluator (*GDExtensionInterfaceVariantGetPtrOperatorEvaluator)(GDExtensionVariantOperator p_operator, GDExtensionVariantType p_type_a, GDExtensionVariantType p_type_b);
typedef GDExtensionPtrBuiltInMethod (*GDExtensionInterfaceVariantGetPtrBuiltinMethod)(GDExtensionVariantType p_type, GDExtensionConstStringNamePtr p_method, GDExtensionInt p_hash);
typedef GDExtensionPtrConstructor (*GDExtensionInterfaceVariantGetPtrConstructor)(GDExtensionVariantType p_type, int32_t p_constructor);
typedef GDExtensionPtrDestructor (*GDExtensionInterfaceVariantGetPtrDestructor)(GDExtensionVariantType p_type);
typedef void (*GDExtensionInterfaceVariantConstruct)(GDExtensionVariantType p_type, GDExtensionVariantPtr p_base, const GDExtensionConstVariantPtr *p_args, int32_t p_argument_count, GDExtensionCallError *r_error);
typedef GDExtensionPtrSetter (*GDExtensionInterfaceVariantGetPtrSetter)(GDExtensionVariantType p_type, GDExtensionConstStringNamePtr p_member);
typedef GDExtensionPtrGetter (*GDExtensionInterfaceVariantGetPtrGetter)(GDExtensionVariantType p_type, GDExtensionConstStringNamePtr p_member);
typedef GDExtensionPtrIndexedSetter (*GDExtensionInterfaceVariantGetPtrIndexedSetter)(GDExtensionVariantType p_type);
typedef GDExtensionPtrIndexedGetter (*GDExtensionInterfaceVariantGetPtrIndexedGetter)(GDExtensionVariantType p_type);
typedef GDExtensionPtrKeyedSetter (*GDExtensionInterfaceVariantGetPtrKeyedSetter)(GDExtensionVariantType p_type);
typedef GDExtensionPtrKeyedGetter (*GDExtensionInterfaceVariantGetPtrKeyedGetter)(GDExtensionVariantType p_type);
typedef GDExtensionPtrKeyedChecker (*GDExtensionInterfaceVariantGetPtrKeyedChecker)(GDExtensionVariantType p_type);
typedef void (*GDExtensionInterfaceVariantGetConstantValue)(GDExtensionVariantType p_type, GDExtensionConstStringNamePtr p_constant, GDExtensionVariantPtr r_ret);
typedef GDExtensionPtrUtilityFunction (*GDExtensionInterfaceVariantGetPtrUtilityFunction)(GDExtensionConstStringNamePtr p_function, GDExtensionInt p_hash);

/* INTERFACE: String Utilities */

typedef void (*GDExtensionInterfaceStringNewWithLatin1Chars)(GDExtensionStringPtr r_dest, const char *p_contents);
typedef void (*GDExtensionInterfaceStringNewWithUtf8Chars)(GDExtensionStringPtr r_dest, const char *p_contents);
typedef void (*GDExtensionInterfaceStringNewWithUtf16Chars)(GDExtensionStringPtr r_dest, const char16_t *p_contents);
typedef void (*GDExtensionInterfaceStringNewWithUtf32Chars)(GDExtensionStringPtr r_dest, const char32_t *p_contents);
typedef void (*GDExtensionInterfaceStringNewWithWideChars)(GDExtensionStringPtr r_dest, const wchar_t *p_contents);
typedef void (*GDExtensionInterfaceStringNewWithLatin1CharsAndLen)(GDExtensionStringPtr r_dest, const char *p_contents, GDExtensionInt p_size);
typedef void (*GDExtensionInterfaceStringNewWithUtf8CharsAndLen)(GDExtensionStringPtr r_dest, const char *p_contents, GDExtensionInt p_size);
typedef void (*GDExtensionInterfaceStringNewWithUtf16CharsAndLen)(GDExtensionStringPtr r_dest, const char16_t *p_contents, GDExtensionInt p_size);
typedef void (*GDExtensionInterfaceStringNewWithUtf32CharsAndLen)(GDExtensionStringPtr r_dest, const char32_t *p_contents, GDExtensionInt p_size);
typedef void (*GDExtensionInterfaceStringNewWithWideCharsAndLen)(GDExtensionStringPtr r_dest, const wchar_t *p_contents, GDExtensionInt p_size);
/* Information about the following functions:
 * - The return value is the resulting encoded string length.
 * - The length returned is in characters, not in bytes. It also does not include a trailing zero.
 * - These functions also do not write trailing zero, If you need it, write it yourself at the position indicated by the length (and make sure to allocate it).
 * - Passing NULL in r_text means only the length is computed (again, without including trailing zero).
 * - p_max_write_length argument is in characters, not bytes. It will be ignored if r_text is NULL.
 * - p_max_write_length argument does not affect the return value, it's only to cap write length.
 */
typedef GDExtensionInt (*GDExtensionInterfaceStringToLatin1Chars)(GDExtensionConstStringPtr p_self, char *r_text, GDExtensionInt p_max_write_length);
typedef GDExtensionInt (*GDExtensionInterfaceStringToUtf8Chars)(GDExtensionConstStringPtr p_self, char *r_text, GDExtensionInt p_max_write_length);
typedef GDExtensionInt (*GDExtensionInterfaceStringToUtf16Chars)(GDExtensionConstStringPtr p_self, char16_t *r_text, GDExtensionInt p_max_write_length);
typedef GDExtensionInt (*GDExtensionInterfaceStringToUtf32Chars)(GDExtensionConstStringPtr p_self, char32_t *r_text, GDExtensionInt p_max_write_length);
typedef GDExtensionInt (*GDExtensionInterfaceStringToWideChars)(GDExtensionConstStringPtr p_self, wchar_t *r_text, GDExtensionInt p_max_write_length);
typedef char32_t *(*GDExtensionInterfaceStringOperatorIndex)(GDExtensionStringPtr p_self, GDExtensionInt p_index);
typedef const char32_t *(*GDExtensionInterfaceStringOperatorIndexConst)(GDExtensionConstStringPtr p_self, GDExtensionInt p_index);

typedef void (*GDExtensionInterfaceStringOperatorPlusEqString)(GDExtensionStringPtr p_self, GDExtensionConstStringPtr p_b);
typedef void (*GDExtensionInterfaceStringOperatorPlusEqChar)(GDExtensionStringPtr p_self, char32_t p_b);
typedef void (*GDExtensionInterfaceStringOperatorPlusEqCstr)(GDExtensionStringPtr p_self, const char *p_b);
typedef void (*GDExtensionInterfaceStringOperatorPlusEqWcstr)(GDExtensionStringPtr p_self, const wchar_t *p_b);
typedef void (*GDExtensionInterfaceStringOperatorPlusEqC32str)(GDExtensionStringPtr p_self, const char32_t *p_b);

/* INTERFACE: XMLParser Utilities */

typedef GDExtensionInt (*GDExtensionInterfaceXmlParserOpenBuffer)(GDExtensionObjectPtr p_instance, const uint8_t *p_buffer, size_t p_size);

/* INTERFACE: FileAccess Utilities */

typedef void (*GDExtensionInterfaceFileAccessStoreBuffer)(GDExtensionObjectPtr p_instance, const uint8_t *p_src, uint64_t p_length);
typedef uint64_t (*GDExtensionInterfaceFileAccessGetBuffer)(GDExtensionConstObjectPtr p_instance, uint8_t *p_dst, uint64_t p_length);

/* INTERFACE: WorkerThreadPool Utilities */

typedef int64_t (*GDExtensionInterfaceWorkerThreadPoolAddNativeGroupTask)(GDExtensionObjectPtr p_instance, void (*p_func)(void *, uint32_t), void *p_userdata, int p_elements, int p_tasks, GDExtensionBool p_high_priority, GDExtensionConstStringPtr p_description);
typedef int64_t (*GDExtensionInterfaceWorkerThreadPoolAddNativeTask)(GDExtensionObjectPtr p_instance, void (*p_func)(void *), void *p_userdata, GDExtensionBool p_high_priority, GDExtensionConstStringPtr p_description);

/* INTERFACE: Packed Array */

typedef uint8_t *(*GDExtensionInterfacePackedByteArrayOperatorIndex)(GDExtensionTypePtr p_self, GDExtensionInt p_index); // p_self should be a PackedByteArray
typedef const uint8_t *(*GDExtensionInterfacePackedByteArrayOperatorIndexConst)(GDExtensionConstTypePtr p_self, GDExtensionInt p_index); // p_self should be a PackedByteArray
typedef GDExtensionTypePtr (*GDExtensionInterfacePackedColorArrayOperatorIndex)(GDExtensionTypePtr p_self, GDExtensionInt p_index); // p_self should be a PackedColorArray, returns Color ptr
typedef GDExtensionTypePtr (*GDExtensionInterfacePackedColorArrayOperatorIndexConst)(GDExtensionConstTypePtr p_self, GDExtensionInt p_index); // p_self should be a PackedColorArray, returns Color ptr
typedef float *(*GDExtensionInterfacePackedFloat32ArrayOperatorIndex)(GDExtensionTypePtr p_self, GDExtensionInt p_index); // p_self should be a PackedFloat32Array
typedef const float *(*GDExtensionInterfacePackedFloat32ArrayOperatorIndexConst)(GDExtensionConstTypePtr p_self, GDExtensionInt p_index); // p_self should be a PackedFloat32Array
typedef double *(*GDExtensionInterfacePackedFloat64ArrayOperatorIndex)(GDExtensionTypePtr p_self, GDExtensionInt p_index); // p_self should be a PackedFloat64Array
typedef const double *(*GDExtensionInterfacePackedFloat64ArrayOperatorIndexConst)(GDExtensionConstTypePtr p_self, GDExtensionInt p_index); // p_self should be a PackedFloat64Array
typedef int32_t *(*GDExtensionInterfacePackedInt32ArrayOperatorIndex)(GDExtensionTypePtr p_self, GDExtensionInt p_index); // p_self should be a PackedInt32Array
typedef const int32_t *(*GDExtensionInterfacePackedInt32ArrayOperatorIndexConst)(GDExtensionConstTypePtr p_self, GDExtensionInt p_index); // p_self should be a PackedInt32Array
typedef int64_t *(*GDExtensionInterfacePackedInt64ArrayOperatorIndex)(GDExtensionTypePtr p_self, GDExtensionInt p_index); // p_self should be a PackedInt32Array
typedef const int64_t *(*GDExtensionInterfacePackedInt64ArrayOperatorIndexConst)(GDExtensionConstTypePtr p_self, GDExtensionInt p_index); // p_self should be a PackedInt32Array
typedef GDExtensionStringPtr (*GDExtensionInterfacePackedStringArrayOperatorIndex)(GDExtensionTypePtr p_self, GDExtensionInt p_index); // p_self should be a PackedStringArray
typedef GDExtensionStringPtr (*GDExtensionInterfacePackedStringArrayOperatorIndexConst)(GDExtensionConstTypePtr p_self, GDExtensionInt p_index); // p_self should be a PackedStringArray
typedef GDExtensionTypePtr (*GDExtensionInterfacePackedVector2ArrayOperatorIndex)(GDExtensionTypePtr p_self, GDExtensionInt p_index); // p_self should be a PackedVector2Array, returns Vector2 ptr
typedef GDExtensionTypePtr (*GDExtensionInterfacePackedVector2ArrayOperatorIndexConst)(GDExtensionConstTypePtr p_self, GDExtensionInt p_index); // p_self should be a PackedVector2Array, returns Vector2 ptr
typedef GDExtensionTypePtr (*GDExtensionInterfacePackedVector3ArrayOperatorIndex)(GDExtensionTypePtr p_self, GDExtensionInt p_index); // p_self should be a PackedVector3Array, returns Vector3 ptr
typedef GDExtensionTypePtr (*GDExtensionInterfacePackedVector3ArrayOperatorIndexConst)(GDExtensionConstTypePtr p_self, GDExtensionInt p_index); // p_self should be a PackedVector3Array, returns Vector3 ptr
typedef GDExtensionVariantPtr (*GDExtensionInterfaceArrayOperatorIndex)(GDExtensionTypePtr p_self, GDExtensionInt p_index); // p_self should be an Array ptr
typedef GDExtensionVariantPtr (*GDExtensionInterfaceArrayOperatorIndexConst)(GDExtensionConstTypePtr p_self, GDExtensionInt p_index); // p_self should be an Array ptr
typedef void (*GDExtensionInterfaceArrayRef)(GDExtensionTypePtr p_self, GDExtensionConstTypePtr p_from); // p_self should be an Array ptr
typedef void (*GDExtensionInterfaceArraySetTyped)(GDExtensionTypePtr p_self, GDExtensionVariantType p_type, GDExtensionConstStringNamePtr p_class_name, GDExtensionConstVariantPtr p_script); // p_self should be an Array ptr

/* INTERFACE: Dictionary */

typedef GDExtensionVariantPtr (*GDExtensionInterfaceDictionaryOperatorIndex)(GDExtensionTypePtr p_self, GDExtensionConstVariantPtr p_key); // p_self should be an Dictionary ptr
typedef GDExtensionVariantPtr (*GDExtensionInterfaceDictionaryOperatorIndexConst)(GDExtensionConstTypePtr p_self, GDExtensionConstVariantPtr p_key); // p_self should be an Dictionary ptr

/* INTERFACE: Object */

typedef void (*GDExtensionInterfaceObjectMethodBindCall)(GDExtensionMethodBindPtr p_method_bind, GDExtensionObjectPtr p_instance, const GDExtensionConstVariantPtr *p_args, GDExtensionInt p_arg_count, GDExtensionVariantPtr r_ret, GDExtensionCallError *r_error);
typedef void (*GDExtensionInterfaceObjectMethodBindPtrcall)(GDExtensionMethodBindPtr p_method_bind, GDExtensionObjectPtr p_instance, const GDExtensionConstTypePtr *p_args, GDExtensionTypePtr r_ret);
typedef void (*GDExtensionInterfaceObjectDestroy)(GDExtensionObjectPtr p_o);
typedef GDExtensionObjectPtr (*GDExtensionInterfaceGlobalGetSingleton)(GDExtensionConstStringNamePtr p_name);
typedef void *(*GDExtensionInterfaceObjectGetInstanceBinding)(GDExtensionObjectPtr p_o, void *p_token, const GDExtensionInstanceBindingCallbacks *p_callbacks);
typedef void (*GDExtensionInterfaceObjectSetInstanceBinding)(GDExtensionObjectPtr p_o, void *p_token, void *p_binding, const GDExtensionInstanceBindingCallbacks *p_callbacks);
typedef void (*GDExtensionInterfaceObjectSetInstance)(GDExtensionObjectPtr p_o, GDExtensionConstStringNamePtr p_classname, GDExtensionClassInstancePtr p_instance); /* p_classname should be a registered extension class and should extend the p_o object's class. */
typedef GDExtensionObjectPtr (*GDExtensionInterfaceObjectCastTo)(GDExtensionConstObjectPtr p_object, void *p_class_tag);
typedef GDExtensionObjectPtr (*GDExtensionInterfaceObjectGetInstanceFromId)(GDObjectInstanceID p_instance_id);
typedef GDObjectInstanceID (*GDExtensionInterfaceObjectGetInstanceId)(GDExtensionConstObjectPtr p_object);

/* INTERFACE: Reference */

typedef GDExtensionObjectPtr (*GDExtensionInterfaceRefGetObject)(GDExtensionConstRefPtr p_ref);
typedef void (*GDExtensionInterfaceRefSetObject)(GDExtensionRefPtr p_ref, GDExtensionObjectPtr p_object);

/* INTERFACE: Script Instance */

typedef GDExtensionScriptInstancePtr (*GDExtensionInterfaceScriptInstanceCreate)(const GDExtensionScriptInstanceInfo *p_info, GDExtensionScriptInstanceDataPtr p_instance_data);

/* INTERFACE: ClassDB */

typedef GDExtensionObjectPtr (*GDExtensionInterfaceClassdbConstructObject)(GDExtensionConstStringNamePtr p_classname); /* The passed class must be a built-in godot class, or an already-registered extension class. In both case, object_set_instance should be called to fully initialize the object. */
typedef GDExtensionMethodBindPtr (*GDExtensionInterfaceClassdbGetMethodBind)(GDExtensionConstStringNamePtr p_classname, GDExtensionConstStringNamePtr p_methodname, GDExtensionInt p_hash);
typedef void *(*GDExtensionInterfaceClassdbGetClassTag)(GDExtensionConstStringNamePtr p_classname);

/* INTERFACE: ClassDB Extension */

/* Provided parameters for `classdb_register_extension_*` can be safely freed once the function returns. */
typedef void (*GDExtensionInterfaceClassdbRegisterExtensionClass)(GDExtensionClassLibraryPtr p_library, GDExtensionConstStringNamePtr p_class_name, GDExtensionConstStringNamePtr p_parent_class_name, const GDExtensionClassCreationInfo *p_extension_funcs);
typedef void (*GDExtensionInterfaceClassdbRegisterExtensionClassMethod)(GDExtensionClassLibraryPtr p_library, GDExtensionConstStringNamePtr p_class_name, const GDExtensionClassMethodInfo *p_method_info);
typedef void (*GDExtensionInterfaceClassdbRegisterExtensionClassIntegerConstant)(GDExtensionClassLibraryPtr p_library, GDExtensionConstStringNamePtr p_class_name, GDExtensionConstStringNamePtr p_enum_name, GDExtensionConstStringNamePtr p_constant_name, GDExtensionInt p_constant_value, GDExtensionBool p_is_bitfield);
typedef void (*GDExtensionInterfaceClassdbRegisterExtensionClassProperty)(GDExtensionClassLibraryPtr p_library, GDExtensionConstStringNamePtr p_class_name, const GDExtensionPropertyInfo *p_info, GDExtensionConstStringNamePtr p_setter, GDExtensionConstStringNamePtr p_getter);
typedef void (*GDExtensionInterfaceClassdbRegisterExtensionClassPropertyGroup)(GDExtensionClassLibraryPtr p_library, GDExtensionConstStringNamePtr p_class_name, GDExtensionConstStringPtr p_group_name, GDExtensionConstStringPtr p_prefix);
typedef void (*GDExtensionInterfaceClassdbRegisterExtensionClassPropertySubgroup)(GDExtensionClassLibraryPtr p_library, GDExtensionConstStringNamePtr p_class_name, GDExtensionConstStringPtr p_subgroup_name, GDExtensionConstStringPtr p_prefix);
typedef void (*GDExtensionInterfaceClassdbRegisterExtensionClassSignal)(GDExtensionClassLibraryPtr p_library, GDExtensionConstStringNamePtr p_class_name, GDExtensionConstStringNamePtr p_signal_name, const GDExtensionPropertyInfo *p_argument_info, GDExtensionInt p_argument_count);
typedef void (*GDExtensionInterfaceClassdbUnregisterExtensionClass)(GDExtensionClassLibraryPtr p_library, GDExtensionConstStringNamePtr p_class_name); /* Unregistering a parent class before a class that inherits it will result in failure. Inheritors must be unregistered first. */

typedef void (*GDExtensionInterfaceGetLibraryPath)(GDExtensionClassLibraryPtr p_library, GDExtensionStringPtr r_path);

/* INITIALIZATION */

typedef enum {
	GDEXTENSION_INITIALIZATION_CORE,
	GDEXTENSION_INITIALIZATION_SERVERS,
	GDEXTENSION_INITIALIZATION_SCENE,
	GDEXTENSION_INITIALIZATION_EDITOR,
	GDEXTENSION_MAX_INITIALIZATION_LEVEL,
} GDExtensionInitializationLevel;

typedef struct {
	/* Minimum initialization level required.
	 * If Core or Servers, the extension needs editor or game restart to take effect */
	GDExtensionInitializationLevel minimum_initialization_level;
	/* Up to the user to supply when initializing */
	void *userdata;
	/* This function will be called multiple times for each initialization level. */
	void (*initialize)(void *userdata, GDExtensionInitializationLevel p_level);
	void (*deinitialize)(void *userdata, GDExtensionInitializationLevel p_level);
} GDExtensionInitialization;

/* Define a C function prototype that implements the function below and expose it to dlopen() (or similar).
 * This is the entry point of the GDExtension library and will be called on initialization.
 * It can be used to set up different init levels, which are called during various stages of initialization/shutdown.
 * The function name must be a unique one specified in the .gdextension config file.
 */
typedef GDExtensionBool (*GDExtensionInitializationFunction)(GDExtensionInterfaceGetProcAddress p_interface, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization);

#ifdef __cplusplus
}
#endif

#endif // GDEXTENSION_INTERFACE_H
