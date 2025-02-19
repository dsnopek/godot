/**************************************************************************/
/*  required_ptr.h                                                        */
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

#ifndef REQUIRED_PTR_H
#define REQUIRED_PTR_H

#include "core/os/memory.h"

template <typename T>
class RequiredPtr {
	T *value = nullptr;

public:
	typedef T *type;

	_FORCE_INLINE_ RequiredPtr(T *p_value) {
		value = p_value;
	}

	_FORCE_INLINE_ RequiredPtr(const RequiredPtr<T> &p_other) {
		value = p_other.value;
	}

	_FORCE_INLINE_ RequiredPtr<T> &operator=(const RequiredPtr<T> &p_other) {
		value = p_other.value;
	}

	_FORCE_INLINE_ RequiredPtr(const Variant &p_value) :
			RequiredPtr(dynamic_cast<T *>((Object *)p_value)) {}

	//
	// DO NOT CALL THIS DIRECTLY!
	//
	// You should use the ERR_FAIL_REQUIRED_PTR*() macros to get the pointer value.
	//
	_FORCE_INLINE_ T *_internal_ptr() const {
		return value;
	}
};

// Specialization for Ref<T>.
template <typename T>
class RequiredPtr<Ref<T>> {
	Ref<T> value;

public:
	typedef Ref<T> type;

	_FORCE_INLINE_ RequiredPtr(const Ref<T> &p_value) {
		value = p_value;
	}

	template <typename U, typename = std::enable_if_t<std::is_base_of_v<T, U>>>
	_FORCE_INLINE_ RequiredPtr(const Ref<U> &p_value) {
		value = p_value;
	}

	_FORCE_INLINE_ RequiredPtr(const RequiredPtr<Ref<T>> &p_other) {
		value = p_other.value;
	}

	_FORCE_INLINE_ RequiredPtr<Ref<T>> &operator=(const RequiredPtr<Ref<T>> &p_other) {
		value = p_other.value;
	}

	_FORCE_INLINE_ RequiredPtr(const Variant &p_value) :
			RequiredPtr(Ref<T>(dynamic_cast<T *>((Object *)p_value))) {}

	//
	// DO NOT CALL THIS DIRECTLY!
	//
	// You should use the ERR_FAIL_REQUIRED_PTR*() macros to get the pointer value.
	//
	_FORCE_INLINE_ Ref<T> _internal_ptr() const {
		return value;
	}
};

template <class T>
struct PtrToArg<RequiredPtr<T>> {
	typedef T *EncodeT;

	_FORCE_INLINE_ static T *convert(const void *p_ptr) {
		if (p_ptr == nullptr) {
			// Should we show an error?
			return RequiredPtr<T>(nullptr);
		}
		return RequiredPtr<T>(*reinterpret_cast<T *const *>(p_ptr));
	}

	_FORCE_INLINE_ static void encode(RequiredPtr<T> p_var, void *p_ptr) {
		*((T **)p_ptr) = p_var._internal_ptr();
	}
};

template <class T>
struct PtrToArg<const RequiredPtr<T> &> {
	typedef T *EncodeT;

	_FORCE_INLINE_ static RequiredPtr<T> convert(const void *p_ptr) {
		if (p_ptr == nullptr) {
			// Should we show an error?
			return RequiredPtr<T>(nullptr);
		}
		return RequiredPtr<T>(*reinterpret_cast<T *const *>(p_ptr));
	}
};

template <typename T>
struct GetTypeInfo<RequiredPtr<T>, std::enable_if_t<std::is_base_of_v<Object, T>>> {
	static const Variant::Type VARIANT_TYPE = Variant::OBJECT;
	static const GodotTypeInfo::Metadata METADATA = GodotTypeInfo::METADATA_OBJECT_IS_REQUIRED;
	static inline PropertyInfo get_class_info() {
		return PropertyInfo(StringName(T::get_class_static()));
	}
};

template <typename T>
struct GetTypeInfo<const RequiredPtr<T> &, std::enable_if_t<std::is_base_of_v<Object, T>>> {
	static const Variant::Type VARIANT_TYPE = Variant::OBJECT;
	static const GodotTypeInfo::Metadata METADATA = GodotTypeInfo::METADATA_OBJECT_IS_REQUIRED;
	static inline PropertyInfo get_class_info() {
		return PropertyInfo(StringName(T::get_class_static()));
	}
};

#endif // REQUIRED_PTR_H
