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

BEGIN_TOVE_NAMESPACE

namespace report {

Configuration config = Configuration{nullptr, TOVE_REPORT_WARN};

static std::string last_warning;

void err(const char *s) {
    report(s, TOVE_REPORT_WARN);
}

void fatal(const char *file, int line, const char *s) {
    std::ostringstream text;
    text << file << ":" << line << ": " << s;
    const std::string output(text.str());
    report(output.c_str(), TOVE_REPORT_FATAL);
}

} // namespace report

END_TOVE_NAMESPACE
