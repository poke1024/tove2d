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

#ifndef __TOVE_INTERFACE
#define __TOVE_INTERFACE 1

#include <cstdint>

#if defined _WIN32 || defined __CYGWIN__
	#define EXPORT __attribute__ ((dllexport))
#else
	#define EXPORT __attribute__ ((visibility ("default")))
#endif

extern "C" {
#include "_interface.h"
}

#endif // __TOVE_INTERFACE
