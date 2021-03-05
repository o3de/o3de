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
