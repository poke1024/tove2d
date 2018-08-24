/*
 * TÖVE - Animated vector graphics for LÖVE.
 * https://github.com/poke1024/tove2d
 *
 * Copyright (c) 2018, Bernhard Liebl
 *
 * Distributed under the MIT license. See LICENSE file for details.
 *
 * All rights reserved.
 */

/*#ifdef TOVE_GODOT
#include "core/reference.h" // from Godot
#endif*/

BEGIN_TOVE_NAMESPACE

#if 1

struct Referencable {
};

template<typename T>
using SharedPtr = std::shared_ptr<T>; 

template<typename T, typename... Args>
SharedPtr<T> tove_make_shared(Args&&... args) {
	return std::make_shared<T>(args...);
}

#elif TOVE_TARGET == TOVE_TARGET_GODOT

typedef ::Reference Referencable;

template <typename T>
class SharedPtr : public ::Ref<T> {
public:
	_FORCE_INLINE_ T *operator->() const {
		return const_cast<T*>(this->ptr());
	}

	T *get() const {
		return const_cast<T*>(this->ptr());
	}

	operator bool() const {
		return this->is_valid();
	}

	SharedPtr &operator=(const SharedPtr &p) {
		this->operator=(p);
		return *this;
	}

	SharedPtr() : Ref<T>() {
	}

	SharedPtr(const SharedPtr &p_from) : Ref<T>(p_from) {
	}

	template <typename T_Other>
	SharedPtr(const SharedPtr<T_Other> &p_from) : Ref<T>(p_from) {
	}

	SharedPtr(T *p_reference) : Ref<T>(p_reference) {
	}
};

template<typename T, typename... Args>
SharedPtr<T> tove_make_shared(Args&&... args) {
	return SharedPtr<T>(memnew(T(args...)));
}
#endif

END_TOVE_NAMESPACE
