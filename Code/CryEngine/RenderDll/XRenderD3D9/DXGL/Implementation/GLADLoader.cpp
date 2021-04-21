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

#include "RenderDll_precompiled.h"

// For definitions
#include <CryDXGL.hpp>

#define NO_INCLUDE_GL_FEATURES
#include <Implementation/GLCommon.hpp>
#undef NO_INCLUDE_GL_FEATURES

// Need to explicitly include glad with the IMPLEMENTATION defines
#if defined(DXGL_USE_LOADER_GLAD)
#   if DXGLES && !defined(DXGL_ES_SUBSET)
#       define GLAD_GLES2_IMPLEMENTATION
#       include <glad/gles2.h>
#       undef GLAD_GLES2_IMPLEMENTATION
#   else
#       define GLAD_GL_IMPLEMENTATION
#       include <glad/gl.h>
#       undef GLAD_GL_IMPLEMENTATION
#   endif
#   if defined(DXGL_USE_WGL)
#       define GLAD_WGL_IMPLEMENTATION
#       include <glad/wgl.h>
#       undef GLAD_WGL_IMPLEMENTATION
#   elif defined(DXGL_USE_EGL)
#       define GLAD_EGL_IMPLEMENTATION
#       include <glad/egl.h>
#       undef GLAD_EGL_IMPLEMENTATION
#   elif defined(DXGL_USE_GLX)
#       define GLAD_GLX_IMPLEMENTATION
#       include <glad/glx.h>
#       undef GLAD_GLX_IMPLEMENTATION
#   endif
#endif
