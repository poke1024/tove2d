/*
 * TÖVE - Animated vector graphics for LÖVE.
 * https://github.com/poke1024/Tove
 *
 * Copyright (c) 2018, Bernhard Liebl
 *
 * Distributed under the MIT license. See LICENSE file for details.
 *
 * All rights reserved.
 */

#include "warn.h"
#include "common.h"

#include <sstream>
#include <iostream>

ToveWarningFunction tove_warn_func = nullptr;

void tove_warn(const char *file, int line, const char *s) {
    std::ostringstream text;
    text << file << ":" << line << ": " << s;
    const std::string output(text.str());
    if (tove_warn_func) {
        tove_warn_func(output.c_str());
    } else {
        std::cerr << "TÖVE: " << output << std::endl;
    }
}
