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

#ifndef __TOVE_WARN
#define __TOVE_WARN 1

#include "common.h"
#include "interface.h"

BEGIN_TOVE_NAMESPACE

namespace report {
    struct Configuration {
        ToveReportFunction report;
        ToveReportLevel level;
    };

    extern Configuration config;

    inline bool warnings() {
        return config.level <= TOVE_REPORT_WARN;
    }

    inline void report(const char *s, ToveReportLevel l) {
        if (l >= config.level && config.report) {
            config.report(s, l);
        }
    }

    inline void warn(const char *s) {
        report(s, TOVE_REPORT_WARN);
    }

    void err(const char *s);
    void fatal(const char *file, int line, const char *s);

} // namespace report 

#define TOVE_FATAL(s) tove::report::fatal(__FILE__, __LINE__, s)
#define TOVE_BAD_ALLOC() { TOVE_FATAL("Out of memory"); throw std::bad_alloc(); }

END_TOVE_NAMESPACE

#endif // __TOVE_WARN
