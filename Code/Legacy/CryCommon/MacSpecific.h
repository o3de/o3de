/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Mac declarations


#ifndef CRYINCLUDE_CRYCOMMON_MACSPECIFIC_H
#define CRYINCLUDE_CRYCOMMON_MACSPECIFIC_H
#pragma once


#include "AppleSpecific.h"
#include <cstddef>
#include <cfloat>
#include <xmmintrin.h>
//#define _CPU_X86
#define _CPU_AMD64
#define _CPU_SSE
#define PLATFORM_64BIT

#define USE_CRT 1
#define SIZEOF_PTR 8

typedef uint64_t threadID;


// curses.h stubs for PDcurses keys
#define PADENTER    KEY_MAX + 1
#define CTL_HOME    KEY_MAX + 2
#define CTL_END     KEY_MAX + 3
#define CTL_PGDN    KEY_MAX + 4
#define CTL_PGUP    KEY_MAX + 5

// stubs for virtual keys, isn't used on Mac
#define VK_UP               0
#define VK_DOWN         0
#define VK_RIGHT        0
#define VK_LEFT         0
#define VK_CONTROL  0
#define VK_SCROLL       0

#define MAC_NOT_IMPLEMENTED assert(false);


typedef enum
{
    eDAContinue,
    eDAIgnore,
    eDAIgnoreAll,
    eDABreak,
    eDAStop,
    eDAReportAsBug
} EDialogAction;

extern EDialogAction MacOSXHandleAssert(const char* condition, const char* file, int line, const char* reason, bool);

#endif // CRYINCLUDE_CRYCOMMON_MACSPECIFIC_H
