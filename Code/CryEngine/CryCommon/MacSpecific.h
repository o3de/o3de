/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

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
