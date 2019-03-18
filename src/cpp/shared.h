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

BEGIN_TOVE_NAMESPACE

struct Referencable {
};

template<typename T>
using SharedPtr = std::shared_ptr<T>; 

template<typename T, typename... Args>
SharedPtr<T> tove_make_shared(Args&&... args) {
	return std::make_shared<T>(args...);
}

END_TOVE_NAMESPACE
