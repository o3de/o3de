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

#ifndef CRYINCLUDE_CRYSYSTEM_SYSTEMINIT_H
#define CRYINCLUDE_CRYSYSTEM_SYSTEMINIT_H
#pragma once


#include "System.h"

#if defined(AZ_PLATFORM_ANDROID)
    #include "AndroidConsole.h"

// let the java code know the native renderer is taking over now
extern "C" DLL_EXPORT void OnEngineRendererTakeover(bool engineSplashActive);
#endif

#include "UnixConsole.h"

#if defined(USE_UNIXCONSOLE)
#if defined(LINUX) && !defined(ANDROID)
extern __attribute__((visibility("default"))) CUNIXConsole* pUnixConsole;
#endif
#endif // USE_UNIXCONSOLE

#endif // CRYINCLUDE_CRYSYSTEM_SYSTEMINIT_H
