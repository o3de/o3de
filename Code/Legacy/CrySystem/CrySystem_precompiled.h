/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Precompiled Header.


#pragma once

#include <AzCore/PlatformIncl.h>

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define STDAFX_H_SECTION_1 1
#define STDAFX_H_SECTION_2 2
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION STDAFX_H_SECTION_1
#include AZ_RESTRICTED_FILE(CrySystem_precompiled_h)
#elif defined(LINUX) // Scrubber friendly negated define pattern
#elif !defined(APPLE)
    #include <memory.h>
    #include <malloc.h>
#endif

//on mac the precompiled header is auto included in every .c and .cpp file, no include line necessary.
//.c files don't like the cpp things in here
#ifdef __cplusplus

#include <vector>

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION STDAFX_H_SECTION_2
#include AZ_RESTRICTED_FILE(CrySystem_precompiled_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(APPLE) // Scrubber friendly negated define pattern
#elif defined(ANDROID) // Scrubber friendly negated define pattern
#elif defined(LINUX)
    #   include <sys/io.h>
#else
    #   include <io.h>
#endif

//#define DEFINE_MODULE_NAME "CrySystem"

#define CRYSYSTEM_EXPORTS

#include <platform.h>

#ifdef WIN32
#include <AzCore/PlatformIncl.h>
#include <tlhelp32.h>
#undef GetCharWidth
#undef GetUserName
#endif

/////////////////////////////////////////////////////////////////////////////
// CRY Stuff ////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#include "Cry_Math.h"
#include <smartptr.h>
#include <Range.h>
#include <StlUtils.h>


inline int RoundToClosestMB(size_t memSize)
{
    // add half a MB and shift down to get closest MB
    return((int) ((memSize + (1 << 19)) >> 20));
}


//////////////////////////////////////////////////////////////////////////
// For faster compilation
//////////////////////////////////////////////////////////////////////////
#include <IRenderer.h>
#include <CryFile.h>
#include <ISystem.h>
#include <IXml.h>
#include <ICmdLine.h>
#include <IConsole.h>
#include <ILog.h>

/////////////////////////////////////////////////////////////////////////////
//forward declarations for common Interfaces.
/////////////////////////////////////////////////////////////////////////////
class ITexture;
struct IRenderer;
struct ISystem;
struct ITimer;
struct IFFont;
struct ICVar;
struct IConsole;
struct IProcess;
namespace AZ::IO
{
    struct IArchive;
}
struct ICryFont;
struct IMovieSystem;
struct IAudioSystem;
struct IPhysicalWorld;

#endif //__cplusplus



