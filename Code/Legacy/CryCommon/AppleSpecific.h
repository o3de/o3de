/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// Description : Apple specific declarations common amongst its products

//////////////////////////////////////////////////////////////////////////
// Standard includes.
//////////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <malloc/malloc.h>
#include <Availability.h>
// Atomic operations , guaranteed to work across all apple platforms
#include <libkern/OSAtomic.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
//////////////////////////////////////////////////////////////////////////

#ifndef __COUNTER__
#define __COUNTER__ __LINE__
#endif

#define PHYSICS_EXPORTS

#define MAP_ANONYMOUS MAP_ANON

//////////////////////////////////////////////////////////////////////////
// Define platform independent types.
//////////////////////////////////////////////////////////////////////////
#include "BaseTypes.h"

#define _PACK __attribute__ ((packed))

#define _PTRDIFF_T_DEFINED 1

#define TARGET_DEFAULT_ALIGN (0x8U)

#ifdef _RELEASE
#define __debugbreak()
#else
#define __debugbreak() ::raise(SIGTRAP)
#endif

#define __assume(x)
