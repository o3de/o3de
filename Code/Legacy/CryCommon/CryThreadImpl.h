/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once


#include <CryThread.h>

// Include architecture specific code.
#if defined(LINUX) || defined(APPLE)
#include <CryThreadImpl_pthreads.h>
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(WIN32) || defined(WIN64)
#include <CryThreadImpl_windows.h>
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
    #include AZ_RESTRICTED_FILE(CryThreadImpl_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
// Put other platform specific includes here!
#endif
