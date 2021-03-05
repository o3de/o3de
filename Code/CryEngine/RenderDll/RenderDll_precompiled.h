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

#pragma once

//on mac the precompiled header is auto included in every .c and .cpp file, no include line necessary.
//.c files don't like the cpp things in here
#ifdef __cplusplus

#include <RenderDll/Common/RendererDefs.h>

#include <Cry_Math.h>
#include <Cry_Geo.h>
#include <StlUtils.h>
#include <RenderDll/Common/DevBuffer.h>

#include <RenderDll/XRenderD3D9/DeviceManager/DeviceManager.h>

#include <VertexFormats.h>

#include <RenderDll/Common/CommonRender.h>
#include <IRenderAuxGeom.h>
#include <RenderDll/Common/Shaders/ShaderComponents.h>
#include <RenderDll/Common/Shaders/Shader.h>
#include <RenderDll/Common/Shaders/CShader.h>
#include <RenderDll/Common/RenderMesh.h>
#include <RenderDll/Common/RenderPipeline.h>
#include <RenderDll/Common/RenderThread.h>

#include <RenderDll/Common/Renderer.h>

#include <RenderDll/Common/Textures/Texture.h>
#include <RenderDll/Common/Shaders/Parser.h>

#include <RenderDll/Common/FrameProfiler.h>
#include <RenderDll/Common/Shadow_Renderer.h>
#include <RenderDll/Common/DeferredRenderUtils.h>
#include <RenderDll/Common/ShadowUtils.h>
#include <RenderDll/Common/WaterUtils.h>

#include <RenderDll/Common/OcclQuery.h>

#include <RenderDll/Common/PostProcess/PostProcess.h>

// All handled render elements (except common ones included in "RendElement.h")
#include <RenderDll/Common/RendElements/CREBeam.h>
#include <RenderDll/Common/RendElements/CREClientPoly.h>
#include <RenderDll/Common/RendElements/CRELensOptics.h>
#include <RenderDll/Common/RendElements/CREHDRProcess.h>
#include <RenderDll/Common/RendElements/CRECloud.h>
#include <RenderDll/Common/RendElements/CREDeferredShading.h>
#include <RenderDll/Common/RendElements/CREMeshImpl.h>

#if !defined(NULL_RENDERER)
#include <RenderDll/XRenderD3D9/DeviceManager/ConstantBufferCache.h>
#include <RenderDll/XRenderD3D9/DeviceManager/DeviceWrapper12.h>
#endif

#include <CryCommon/FrameProfiler.h>
#include <AzCore/Debug/EventTrace.h>

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define STDAFX_H_SECTION_1 1
#define STDAFX_H_SECTION_2 2
#define STDAFX_H_SECTION_3 3
#endif

/*-----------------------------------------------------------------------------
    Vector transformations.
-----------------------------------------------------------------------------*/

_inline void TransformVector(Vec3& out, const Vec3& in, const Matrix44A& m)
{
    out.x = in.x * m(0, 0) + in.y * m(1, 0) + in.z * m(2, 0);
    out.y = in.x * m(0, 1) + in.y * m(1, 1) + in.z * m(2, 1);
    out.z = in.x * m(0, 2) + in.y * m(1, 2) + in.z * m(2, 2);
}

_inline void TransformPosition(Vec3& out, const Vec3& in, const Matrix44A& m)
{
    TransformVector (out, in, m);
    out += m.GetRow(3);
}


inline Plane TransformPlaneByUsingAdjointT(const Matrix44A& M, const Matrix44A& TA, const Plane plSrc)
{
    Vec3 newNorm;
    TransformVector (newNorm, plSrc.n, TA);
    newNorm.Normalize();

    if (M.Determinant() < 0.f)
    {
        newNorm *= -1;
    }

    Plane plane;
    Vec3 p;
    TransformPosition (p, plSrc.n * plSrc.d, M);
    plane.Set(newNorm, p | newNorm);

    return plane;
}

inline Matrix44 TransposeAdjoint(const Matrix44A& M)
{
    Matrix44 ta;

    ta(0, 0) = M(1, 1) * M(2, 2) - M(2, 1) * M(1, 2);
    ta(1, 0) = M(2, 1) * M(0, 2) - M(0, 1) * M(2, 2);
    ta(2, 0) = M(0, 1) * M(1, 2) - M(1, 1) * M(0, 2);

    ta(0, 1) = M(1, 2) * M(2, 0) - M(2, 2) * M(1, 0);
    ta(1, 1) = M(2, 2) * M(0, 0) - M(0, 2) * M(2, 0);
    ta(2, 1) = M(0, 2) * M(1, 0) - M(1, 2) * M(0, 0);

    ta(0, 2) = M(1, 0) * M(2, 1) - M(2, 0) * M(1, 1);
    ta(1, 2) = M(2, 0) * M(0, 1) - M(0, 0) * M(2, 1);
    ta(2, 2) = M(0, 0) * M(1, 1) - M(1, 0) * M(0, 1);

    ta(0, 3) = 0.f;
    ta(1, 3) = 0.f;
    ta(2, 3) = 0.f;


    return ta;
}

inline Plane TransformPlane(const Matrix44A& M, const Plane& plSrc)
{
    Matrix44 tmpTA = TransposeAdjoint(M);
    return TransformPlaneByUsingAdjointT(M, tmpTA, plSrc);
}

// Homogeneous plane transform.
inline Plane TransformPlane2(const Matrix34A& m, const Plane& src)
{
    Plane plDst;

    float v0 = src.n.x, v1 = src.n.y, v2 = src.n.z, v3 = src.d;
    plDst.n.x = v0 * m(0, 0) + v1 * m(1, 0) + v2 * m(2, 0);
    plDst.n.y = v0 * m(0, 1) + v1 * m(1, 1) + v2 * m(2, 1);
    plDst.n.z = v0 * m(0, 2) + v1 * m(1, 2) + v2 * m(2, 2);

    plDst.d = v0 * m(0, 3) + v1 * m(1, 3) + v2 * m(2, 3) + v3;

    return plDst;
}

// Homogeneous plane transform.
inline Plane TransformPlane2(const Matrix44A& m, const Plane& src)
{
    Plane plDst;

    float v0 = src.n.x, v1 = src.n.y, v2 = src.n.z, v3 = src.d;
    plDst.n.x = v0 * m(0, 0) + v1 * m(0, 1) + v2 * m(0, 2) + v3 * m(0, 3);
    plDst.n.y = v0 * m(1, 0) + v1 * m(1, 1) + v2 * m(1, 2) + v3 * m(1, 3);
    plDst.n.z = v0 * m(2, 0) + v1 * m(2, 1) + v2 * m(2, 2) + v3 * m(2, 3);

    plDst.d = v0 * m(3, 0) + v1 * m(3, 1) + v2 * m(3, 2) + v3 * m(3, 3);

    return plDst;
}
inline Plane TransformPlane2_NoTrans(const Matrix44A& m, const Plane& src)
{
    Plane plDst;
    TransformVector(plDst.n, src.n, m);
    plDst.d = src.d;

    return plDst;
}

inline Plane TransformPlane2Transposed(const Matrix44A& m, const Plane& src)
{
    Plane plDst;

    float v0 = src.n.x, v1 = src.n.y, v2 = src.n.z, v3 = src.d;
    plDst.n.x = v0 * m(0, 0) + v1 * m(1, 0) + v2 * m(2, 0) + v3 * m(3, 0);
    plDst.n.y = v0 * m(0, 1) + v1 * m(1, 1) + v2 * m(2, 1) + v3 * m(3, 1);
    plDst.n.z = v0 * m(0, 2) + v1 * m(2, 1) + v2 * m(2, 2) + v3 * m(3, 2);

    plDst.d   = v0 * m(0, 3) + v1 * m(1, 3) + v2 * m(2, 3) + v3 * m(3, 3);

    return plDst;
}

//===============================================================================================

#define MAX_PATH_LENGTH 512

#if defined(LINUX) ||  defined(APPLE)
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION STDAFX_H_SECTION_1
    #include AZ_RESTRICTED_FILE(RenderDll_precompiled_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else   //than it does already exist
inline int vsnprintf(char* buf, int size, const char* format, va_list& args)
{
    int res = azvsnprintf(buf, size, format, args);
    assert(res >= 0 && res < size); // just to know if there was problems in past
    buf[size - 1] = 0;
    return res;
}
#endif

#if defined(LINUX) || defined(APPLE)
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION STDAFX_H_SECTION_2
    #include AZ_RESTRICTED_FILE(RenderDll_precompiled_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else   //than it does already exist
inline int snprintf(char* buf, int size, const char* format, ...)
{
    va_list arglist;
    va_start(arglist, format);
    int res = azvsnprintf(buf, size, format, arglist);
    va_end(arglist);
    return res;
}
#endif

//////////////////////////////////////////////////////////////////////////
// Report warning to validator.
//////////////////////////////////////////////////////////////////////////
inline void Warning(const char* format, ...) PRINTF_PARAMS(1, 2);
inline void Warning(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    if (iSystem)
    {
        iSystem->WarningV(VALIDATOR_MODULE_RENDERER, VALIDATOR_WARNING, 0, NULL, format, args);
    }
    va_end(args);
}

//////////////////////////////////////////////////////////////////////////
// Report warning to validator.
//////////////////////////////////////////////////////////////////////////
inline void LogWarning(const char* format, ...) PRINTF_PARAMS(1, 2);
inline void LogWarning(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    if (iSystem)
    {
        iSystem->WarningV(VALIDATOR_MODULE_RENDERER, VALIDATOR_WARNING, 0, NULL, format, args);
    }
    va_end(args);
}

inline void LogWarningEngineOnly(const char* format, ...) PRINTF_PARAMS(1, 2);
inline void LogWarningEngineOnly(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    if (iSystem)
    {
        iSystem->WarningV(VALIDATOR_MODULE_RENDERER, VALIDATOR_WARNING, VALIDATOR_FLAG_IGNORE_IN_EDITOR, NULL, format, args);
    }
    va_end(args);
}

//////////////////////////////////////////////////////////////////////////
// Report warning to validator.
//////////////////////////////////////////////////////////////////////////
inline void FileWarning(const char* filename, const char* format, ...) PRINTF_PARAMS(2, 3);
inline void FileWarning(const char* filename, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    if (iSystem)
    {
        iSystem->WarningV(VALIDATOR_MODULE_RENDERER, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE, filename, format, args);
    }
    va_end(args);
}

//////////////////////////////////////////////////////////////////////////
// Report warning to validator.
//////////////////////////////////////////////////////////////////////////
inline void TextureWarning(const char* filename, const char* format, ...) PRINTF_PARAMS(2, 3);
inline void TextureWarning(const char* filename, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    if (iSystem)
    {
        iSystem->WarningV(VALIDATOR_MODULE_RENDERER, VALIDATOR_WARNING, (VALIDATOR_FLAG_FILE | VALIDATOR_FLAG_TEXTURE), filename, format, args);
    }
    va_end(args);
}

inline void TextureError(const char* filename, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    if (iSystem)
    {
        iSystem->WarningV(VALIDATOR_MODULE_RENDERER, VALIDATOR_ERROR, (VALIDATOR_FLAG_FILE | VALIDATOR_FLAG_TEXTURE), filename, format, args);
    }
    va_end(args);
}

_inline void _SetVar(const char* szVarName, int nVal)
{
    ICVar* var = iConsole->GetCVar(szVarName);
    if (var)
    {
        var->Set(nVal);
    }
    else
    {
        assert(0);
    }
}

// Get the sub-string starting at the last . in the string, or NULL if the string contains no dot
// Note: The returned pointer refers to a location inside the provided string, no allocation is performed
const char* fpGetExtension (const char* in);

// Remove extension from string, including the .
// If the string has no extension, the whole string will be copied into the buffer
// Note: The out buffer must have space to store a copy of the in-string and a null-terminator
void fpStripExtension (const char* in, char* out, size_t bytes);
template<size_t bytes>
void fpStripExtension (const char* in, char (&out)[bytes]) { fpStripExtension(in, out, bytes); }

// Adds an extension to the path, if an extension is already present the function does nothing
// The extension should include the .
// Note: The path buffer must have enough unused space to store a copy of the extension string
void fpAddExtension (char* path, const char* extension, size_t bytes);
template<size_t bytes>
void fpAddExtension (char (&path)[bytes], const char* extension) { fpAddExtension(path, extension, bytes); }

// Converts DOS slashes to UNIX slashes
// Note: The dst buffer must have space to store a copy of src and a null-terminator
void fpConvertDOSToUnixName (char* dst, const char* src, size_t bytes);
template<size_t bytes>
void fpConvertDOSToUnixName (char (&dst)[bytes], const char* src) { fpConvertDOSToUnixName(dst, src, bytes); }

// Converts UNIX slashes to DOS slashes
// Note: the dst buffer must have space to store a copy of src and a null-terminator
void fpConvertUnixToDosName (char* dst, const char* src, size_t bytes);
template<size_t bytes>
void fpConvertUnixToDosName (char (&dst)[bytes], const char* src) { fpConvertUnixToDosName(dst, src, bytes); }

// Combines the path and name strings, inserting a UNIX slash as required, and stores the result into the dst buffer
// path may be NULL, in which case name will be copied into the dst buffer, and the UNIX slash is NOT inserted
// Note: the dst buffer must have space to store: a copy of name, a copy of path (if not null), a UNIX slash (if path doesn't end with one) and a null-terminator
void fpUsePath (const char* name, const char* path, char* dst, size_t bytes);
template<size_t bytes>
void fpUsePath (const char* name, const char* path, char (&dst)[bytes]) { fpUsePath(name, path, dst, bytes); }

//=========================================================================================
//
// Normal timing.
//
#define ticks(Timer)   {Timer -= CryGetTicks(); }
#define unticks(Timer) {Timer += CryGetTicks() + 34; }

//=============================================================================

// the int 3 call for 32-bit version for .l-generated files.
#ifdef WIN64
#define LEX_DBG_BREAK
#else
#define LEX_DBG_BREAK DEBUG_BREAK
#endif

#include <RenderDll/Common/Defs.h>

#define FUNCTION_PROFILER_RENDERER FUNCTION_PROFILER_FAST(iSystem, PROFILE_RENDERER, g_bProfilerEnabled)

#define SCOPED_RENDERER_ALLOCATION_NAME_HINT(str)

#define SHADER_ASYNC_COMPILATION

#if !defined(_RELEASE)
//# define DETAILED_PROFILING_MARKERS
#endif
#if defined(DETAILED_PROFILING_MARKERS)
# define DETAILED_PROFILE_MARKER(x) DETAILED_PROFILE_MARKER((x))
#else
# define DETAILED_PROFILE_MARKER(x) (void)0
#endif

#define RAIN_OCC_MAP_SIZE           256

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION STDAFX_H_SECTION_3
    #include AZ_RESTRICTED_FILE(RenderDll_precompiled_h)
#endif

#if defined(WIN32)

#include <float.h>

struct ScopedSetFloatExceptionMask
{
    ScopedSetFloatExceptionMask(unsigned int disable = _EM_INEXACT | _EM_UNDERFLOW | _EM_OVERFLOW | _EM_DENORMAL | _EM_INVALID)
    {
        _clearfp();
        _controlfp_s(&oldMask, 0, 0);
        unsigned temp;
        _controlfp_s(&temp, disable, _MCW_EM);
    }
    ~ScopedSetFloatExceptionMask()
    {
        _clearfp();
        unsigned temp;
        _controlfp_s(&temp, oldMask, _MCW_EM);
    }
    unsigned oldMask;
};

#define SCOPED_ENABLE_FLOAT_EXCEPTIONS ScopedSetFloatExceptionMask scopedSetFloatExceptionMask(0)
#define SCOPED_DISABLE_FLOAT_EXCEPTIONS ScopedSetFloatExceptionMask scopedSetFloatExceptionMask

#endif  // defined(_MSC_VER)
#include <RenderDll/XRenderD3D9/DeviceManager/DeviceManagerInline.h>
#endif //__cplusplus

/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/
