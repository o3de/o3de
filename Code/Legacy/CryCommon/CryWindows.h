/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Specific header to handle Windows.h include


#ifndef CRYINCLUDE_CRYCOMMON_CRYWINDOWS_H
#define CRYINCLUDE_CRYCOMMON_CRYWINDOWS_H
#pragma once


#if defined(WIN32) || defined(WIN64)

#ifndef FULL_WINDOWS_HEADER
    #define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#undef GetCommandLine
#undef GetObject
#undef PlaySound
#undef GetClassName
#undef DrawText
#undef GetCharWidth
#undef GetUserName
#undef LoadLibrary
#undef RegisterClass

#undef min
#undef max

#undef NOMINMAX
#undef WIN32_LEAN_AND_MEAN
#undef FULL_WINDOWS_HEADER

#endif

#endif // CRYINCLUDE_CRYCOMMON_CRYWINDOWS_H
