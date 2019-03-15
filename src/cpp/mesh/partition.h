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

#ifndef __TOVE_MESH_PARTITION
#define __TOVE_MESH_PARTITION 1

#include "../common.h"
#include "utils.h"
#include <vector>

BEGIN_TOVE_NAMESPACE

class Partition {
private:
	typedef std::vector<uint16_t> Indices;

	struct Part {
		Indices outline;
		int fail;
	};

	std::vector<Part> parts;

public:
	inline Partition() {
	}

	Partition(const std::list<TPPLPoly> &convex);

	inline bool empty() const {
		return parts.empty();
	}

	bool check(const Vertices &vertices);
};

END_TOVE_NAMESPACE

#endif // __TOVE_MESH_PARTITION
