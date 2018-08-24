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

#ifndef __TOVE_OBSERVER
#define __TOVE_OBSERVER 1

#include "common.h"
#include <unordered_set>

BEGIN_TOVE_NAMESPACE

class Observable;

class Observer {
public:
    virtual ~Observer() {
    }

	virtual void observableChanged(
        Observable *observable, ToveChangeFlags what) = 0;
};

class Observable {
	std::unordered_set<Observer*> observers;

protected:
    inline bool hasObservers() const {
        return observers.size() > 0;
    }

public:
    virtual ~Observable() {
        assert(!hasObservers());
    }

	inline void addObserver(Observer *observer) {
        observers.insert(observer);        
    }
	inline void removeObserver(Observer *observer) {
        observers.erase(observer);
    }

	void broadcastChange(ToveChangeFlags what) {
        for (Observer *observer : observers) {
            observer->observableChanged(this, what);
        }        
    }
};

END_TOVE_NAMESPACE

#endif // __TOVE_OBSERVER
