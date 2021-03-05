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

// Description : Declares the types and functions that implement the
//               OpenGL rendering functionality of DXGL


#ifndef __GLCOMMON__
#define __GLCOMMON__

#include "GLPlatform.hpp"

#define DXGLES DXGLES_REQUIRED_VERSION > 0

//This is also defined in CryDXGL.hpp
#if DXGLES && !defined(OPENGL_ES)
    #define OPENGL_ES
#endif //DXGLES && !defined(OPENGL_ES)

#ifdef WIN32
#include <WinGDI.h>
#endif //WIN32

#define DXGL_NSIGHT_VERSION_3_0      30
#define DXGL_NSIGHT_VERSION_3_1      31
#define DXGL_NSIGHT_VERSION_3_2      32
#define DXGL_NSIGHT_VERSION_4_0      40
#define DXGL_NSIGHT_VERSION_4_1      41
#define DXGL_NSIGHT_VERSION_4_5      45

#define DXGL_SUPPORT_NSIGHT_VERSION  0
#define DXGL_SUPPORT_APITRACE        0
#define DXGL_SUPPORT_VOGL            0
#define DXGL_GLSL_FROM_HLSLCROSSCOMPILER 1
// Uncomment to enable temporary fix for AMD drivers with a fixed number of texture units
// #define DXGL_MAX_TEXTURE_UNITS 32

#define DXGL_SUPPORT_NSIGHT(_Version) (DXGL_SUPPORT_NSIGHT_VERSION == DXGL_NSIGHT_VERSION_ ## _Version)
#define DXGL_SUPPORT_NSIGHT_SINCE(_Version) (DXGL_SUPPORT_NSIGHT_VERSION && DXGL_SUPPORT_NSIGHT_VERSION <= DXGL_NSIGHT_VERSION_ ## _Version)

#define DXGL_TRACE_CALLS 0
#define DXGL_TRACE_CALLS_FLUSH 0
#define DXGL_CHECK_ERRORS 0

#ifndef NO_INCLUDE_GL_FEATURES
#include "GLFeatures.hpp"

#if defined(AZ_RESTRICTED_PLATFORM)
    #include AZ_RESTRICTED_FILE(GLCommon_hpp)
#endif

namespace NCryOpenGL
{
#if defined(DXGL_USE_EGL)
    struct SDisplayConnection;
    typedef _smart_ptr<SDisplayConnection> TWindowContext;
    typedef EGLContext TRenderingContext;
#elif defined(WIN32)
    typedef HDC TWindowContext;
    typedef HGLRC TRenderingContext;
#else
#error "Platform specific context not defined"
#endif
}

#endif // NO_INCLUDE_GL_FEATURES

#endif //__GLCOMMON__

