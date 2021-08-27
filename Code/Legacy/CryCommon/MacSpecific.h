/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

typedef uint64_t threadID;

#define VK_CONTROL  0

#endif // CRYINCLUDE_CRYCOMMON_MACSPECIFIC_H
