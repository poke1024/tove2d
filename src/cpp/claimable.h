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

#ifndef __TOVE_CLAIMABLE
#define __TOVE_CLAIMABLE 1

template<typename T>
class Claimable {
protected:
	T *claimer;

public:
	inline Claimable() : claimer(nullptr) {
	}

	inline T *getClaimer() const {
		return claimer;
	}

	inline void claim(T *claimer) {
		this->claimer = claimer;
	}

	inline void unclaim(T *claimer) {
		if (this->claimer == claimer) {
			this->claimer = nullptr;
		}
	}

	inline bool isClaimed() const {
		return claimer;
	}
};

#endif // __TOVE_CLAIMABLE
