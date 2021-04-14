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

#ifndef _WINSOCKAPI_
#   define _WINSOCKAPI_
#   define _DID_SKIP_WINSOCK_
#endif

#include <vector>
#include <array>
#include <list>
#include <map>
#include <set>
#include <algorithm>
#include <memory>

#include <platform.h>
#include <CrySizer.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Casting/lossy_cast.h>

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define RENDERERDEFS_H_SECTION_1 1
#define RENDERERDEFS_H_SECTION_2 2
#define RENDERERDEFS_H_SECTION_3 3
#define RENDERERDEFS_H_SECTION_4 4
#define RENDERERDEFS_H_SECTION_5 5
#define RENDERERDEFS_H_SECTION_6 6
#define RENDERERDEFS_H_SECTION_7 7
#define RENDERERDEFS_H_SECTION_8 8
#define RENDERERDEFS_H_SECTION_9 9
#define RENDERERDEFS_H_SECTION_10 10
#define RENDERERDEFS_H_SECTION_11 11
#define RENDERERDEFS_H_SECTION_12 12
#define RENDERERDEFS_H_SECTION_13 13
#define RENDERERDEFS_H_SECTION_14 14
#define RENDERERDEFS_H_SECTION_15 15
#define RENDERERDEFS_H_SECTION_16 16
#define RENDERERDEFS_H_SECTION_17 17
#define RENDERERDEFS_H_SECTION_18 18
#define RENDERERDEFS_H_SECTION_19 19
#define RENDERERDEFS_H_SECTION_20 20
#endif

#define SUPPORTS_WINDOWS_10_SDK false
#if _MSC_VER
#include <ntverp.h>
#undef SUPPORTS_WINDOWS_10_SDK
#define SUPPORTS_WINDOWS_10_SDK (VER_PRODUCTBUILD > 9600)
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERERDEFS_H_SECTION_1
    #include AZ_RESTRICTED_FILE(RendererDefs_h)
#else
#if defined(WIN32) || defined(WIN64) || defined(LINUX) || defined(APPLE)
#define RENDERERDEFS_H_TRAIT_SUPPORT_BAKED_MESHES_AND_DECALS 1
#endif

#define RENDERERDEFS_H_TRAIT_CONSTANT_BUFFER_ENABLE_DIRECT_ACCESS 0

#if defined(WIN32) || defined(WIN64)
#define RENDERERDEFS_H_TRAIT_SUPPORT_D3D_DEBUG_RUNTIME 1
#endif

#if defined(APPLE)
#define RENDERERDEFS_H_TRAIT_DEFINE_D3DPOOL 1
#endif

#define RENDERERDEFS_H_TRAIT_ALIAS_D3DOK 0
#define RENDERMESH_CPP_TRAIT_BUFFER_ENABLE_DIRECT_ACCESS 0

#if defined(MOBILE)
#define RENDERTHREAD_H_TRAIT_USE_LOCKS_FOR_FLUSH_SYNC 1
#endif

#if defined(WIN64)
#define PLANNINGTEXTURESTREAMER_JOBS_CPP_TRAIT_JOB_INITKEYS_PREFETCH 1
#endif

#define TEXTURESTREAMING_CPP_TRAIT_CANSTREAMINPLACE_ETT_2D_EARLY_OUT 1
#define TEXTURESTREAMING_CPP_TRAIT_CANSTREAMINPLACE_FORMATCOMPATIBLE 1
#define TEXTURESTREAMING_CPP_TRAIT_TRYCOMMIT_COPYMIPS 1
#define TEXTURESTREAMING_CPP_TRAIT_COPYMIPS_MOVEENGINE 0
#define TEXTURESTREAMING_CPP_TRAIT_CANSTREAMINPLACE_SRCTILEMODE_CHECK 0

#define D3DFXPIPELINE_CPP_TRAIT_CLEARVIEW 1

#if defined(IOS)
#define D3DGPUPARTICLEENGINE_CPP_TRAIT_BEGINUPDATE_INIT_COMPUTE 1
#endif

#define D3DHWSHADER_H_TRAIT_DEFINE_D3D_BLOB 0

#if defined(OPENGL) || defined(CRY_USE_METAL)
#define D3DHWSHADERCOMPILING_CPP_TRAIT_VERTEX_FORMAT 1
#else
#define D3DHWSHADERCOMPILING_CPP_TRAIT_VERTEX_FORMAT 0
#endif

#if defined(WIN32) || defined(WIN64) || defined(APPLE) || defined(LINUX)
#define D3DRENDERRE_CPP_TRAIT_MFDRAW_SETDEPTHSURF 1
#endif

#define D3DRENDERRE_CPP_TRAIT_MFDRAW_USEINSTANCING 1

#if defined(AZ_PLATFORM_WINDOWS)
#define D3DSTEREO_CPP_TRAIT_SELECTDEFAULTDEVICE_STEREODEVICEDRIVER 1
#endif

#if defined(WIN32)
#define D3DSYSTEM_CPP_TRAIT_SETWINDOW_REGISTERWINDOWMESSAGEHANDLER 1
#endif

#define DRIVERD3D_CPP_TRAIT_CALCULATERESOLUTIONS_1080 0

#if defined(IOS) || defined(ANDROID)
#define DRIVERD3D_CPP_TRAIT_HANDLEDISPLAYPROPERTYCHANGES_FULLSCREEN 1
#endif

#if defined(IOS) || defined(ANDROID)
#define DRIVERD3D_CPP_TRAIT_HANDLEDISPLAYPROPERTYCHANGES_NATIVERES 1
#endif

#define DRIVERD3D_CPP_TRAIT_HANDLEDISPLAYPROPERTYCHANGES_CALCRESOLUTIONS 1

#define DRIVERD3D_CPP_TRAIT_RT_ENDFRAME_NOTIMPL 0

#if defined(_WIN32)
#define DRIVERD3D_CPP_TRAIT_ONSYSTEMEVENT_EVENTMOVE 1
#endif

#if defined (WIN32)
#define DRIVERD3D_H_TRAIT_DEFSAVETEXTURE 1
#endif

#if defined(WIN32)
#define DRIVERD3D_H_TRAIT_DEFREGISTEREDWINDOWHANDLER 1
#endif

#if defined (WIN32) || defined(WIN64) || defined(APPLE) || defined(LINUX)
#define DRIVERD3D_H_TRAIT_DEFOCCLUSIONTEXTURESVALID 1
#endif

#if defined(WIN32)
#define GPUTIMER_CPP_TRAIT_CSIMPLEGPUTIMER_SETQUERYSTART 1
#endif

#if defined(WIN32)
#define GPUTIMER_CPP_TRAIT_RELEASE_RELEASEQUERY 1
#endif

#if defined(WIN32)
#define GPUTIMER_CPP_TRAIT_INIT_CREATEQUERY 1
#endif

#if defined(WIN32)
#define GPUTIMER_CPP_TRAIT_START_PRIMEQUERY 1
#endif

#if defined(WIN32)
#define GPUTIMER_CPP_TRAIT_STOP_ENDQUERY 1
#endif

#if defined(WIN32)
#define GPUTIMER_CPP_TRAIT_UPDATETIME_GETDATA 1
#endif

#if defined(WIN32)
#define GPUTIMER_H_TRAIT_DEFINEQUERIES 1
#endif

#if defined(LINUX) || defined(APPLE)
#define NULL_SYSTEM_TRAIT_INIT_RETURNTHIS 1
#endif

#endif

#if defined(WIN64) && defined (CRY_USE_DX12)
#define CRY_INTEGRATE_DX12
#endif

#ifdef _DEBUG
#define GFX_DEBUG
#endif

//defined in DX9, but not DX10
#if RENDERERDEFS_H_TRAIT_ALIAS_D3DOK || defined(OPENGL)
#define D3D_OK  S_OK
#endif

#if !defined(CRY_USE_DX12) && !defined(OPENGL) && !defined(_RELEASE)        
# define RENDERER_ENABLE_BREAK_ON_ERROR 0       ///< Define causes compile errors on DX12 and OpenGL
#endif
#if !defined(RENDERER_ENABLE_BREAK_ON_ERROR)
# define RENDERER_ENABLE_BREAK_ON_ERROR 0
#endif
#if RENDERER_ENABLE_BREAK_ON_ERROR
# include <winerror.h>
namespace detail
{
    const char* ToString(long const hr);
    bool CheckHResult(long const hr, bool breakOnError, const char* file, const int line);
}
//# undef FAILED
//# define FAILED(x) (detail::CheckHResult((x), false, __FILE__, __LINE__))
# define CHECK_HRESULT(x) (detail::CheckHResult((x), true, __FILE__, __LINE__))
#else
# define CHECK_HRESULT(x) (!FAILED(x))
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERERDEFS_H_SECTION_6
    #include AZ_RESTRICTED_FILE(RendererDefs_h)
#endif

#if defined(OPENGL) && (defined(DEBUG) || defined(_DEBUG))
#define LY_ENABLE_OPENGL_ERROR_CHECKING
#endif
namespace O3de
{
    namespace OpenGL
    {
#if defined(LY_ENABLE_OPENGL_ERROR_CHECKING)
        unsigned int CheckError();
        void ClearErrors();
#else
        inline unsigned int CheckError() { return 0; }
        inline void ClearErrors() {}
#endif
    }
}

// enable support for D3D11.1 features if the platform supports it
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERERDEFS_H_SECTION_2
    #include AZ_RESTRICTED_FILE(RendererDefs_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(CRY_USE_DX12)
#   define DEVICE_SUPPORTS_D3D11_1
#endif

#ifdef _DEBUG
#define CRTDBG_MAP_ALLOC
#endif //_DEBUG

#undef USE_STATIC_NAME_TABLE
#define USE_STATIC_NAME_TABLE

#if !defined(_RELEASE)
#define ENABLE_FRAME_PROFILER
#endif

#if !defined(NULL_RENDERER) && (!defined(_RELEASE) || defined(PERFORMANCE_BUILD))
#define ENABLE_PROFILING_GPU_TIMERS
#define ENABLE_FRAME_PROFILER_LABELS
#endif

#ifdef ENABLE_FRAME_PROFILER
#   define PROFILE 1
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERERDEFS_H_SECTION_7
    #include AZ_RESTRICTED_FILE(RendererDefs_h)
#endif

#ifdef ENABLE_SCUE_VALIDATION
enum EVerifyType
{
    eVerify_Normal,
    eVerify_ConstantBuffer,
    eVerify_VertexBuffer,
    eVerify_SRVTexture,
    eVerify_SRVBuffer,
    eVerify_UAVTexture,
    eVerify_UAVBuffer,
};
#endif

#if __HAS_SSE__
// <fvec.h> includes <assert.h>, include it before platform.h
#include <fvec.h>
#define CONST_INT32_PS(N, V3, V2, V1, V0)                                 \
    const _MM_ALIGN16 int _##N[] = { V0, V1, V2, V3 }; /*little endian!*/ \
    const F32vec4 N = _mm_load_ps((float*)_##N);
#endif

// enable support for baked meshes and decals on PC
#if RENDERERDEFS_H_TRAIT_SUPPORT_BAKED_MESHES_AND_DECALS
//#define RENDER_MESH_TRIANGLE_HASH_MAP_SUPPORT // CryTek removed this #define in 3.8.  Not sure if we should strip out all related code or not, or if it may be a useful feature.
#define TEXTURE_GET_SYSTEM_COPY_SUPPORT
#endif

#if defined(_CPU_SSE) && !defined(WIN64)
#define __HAS_SSE__ 1
#endif

#define MAX_REND_RECURSION_LEVELS 2

#if defined(OPENGL)
#define CRY_OPENGL_ADAPT_CLIP_SPACE 1
#define CRY_OPENGL_FLIP_Y 1
#define CRY_OPENGL_MODIFY_PROJECTIONS !CRY_OPENGL_ADAPT_CLIP_SPACE
#endif // defined(OPENGL)

#ifdef STRIP_RENDER_THREAD
#define m_nCurThreadFill 0
#define m_nCurThreadProcess 0
#endif

#ifdef STRIP_RENDER_THREAD
#define ASSERT_IS_RENDER_THREAD(rt)
#define ASSERT_IS_MAIN_THREAD(rt)
#else
#define ASSERT_IS_RENDER_THREAD(rt) assert((rt)->IsRenderThread());
#define ASSERT_IS_MAIN_THREAD(rt) assert((rt)->IsMainThread());
#define ASSERT_IS_MAIN_OR_RENDER_THREAD(rt) assert((rt)->IsMainThread() || (rt)->IsRenderThread());
#endif

//#define ASSERT_IN_SHADER( expr ) assert( expr );
#define ASSERT_IN_SHADER(expr)

#define _USE_MATH_DEFINES
#include <math.h>

// windows desktop API available for usage
#if defined(WIN32) || defined(WIN64)
#define WINDOWS_DESKTOP_API
#endif

#if (defined(WIN32) || defined(WIN64)) && !defined(OPENGL)
#define LEGACY_D3D9_INCLUDE
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERERDEFS_H_SECTION_3
    #include AZ_RESTRICTED_FILE(RendererDefs_h)
#endif

#if !defined(NULL_RENDERER)

// all D3D10 blob related functions and struct will be deprecated in next DirectX APIs
// and replaced with regular D3DBlob counterparts
#define D3D10CreateBlob                                      D3DCreateBlob

// Direct3D11 includes
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERERDEFS_H_SECTION_9
    #include AZ_RESTRICTED_FILE(RendererDefs_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(OPENGL)
#include "CryLibrary.h"
//  enable metal for ios device only. Emulator is not supported for now.
#if TARGET_OS_IPHONE || TARGET_OS_TV
#ifndef __IPHONE_9_0
#define __IPHONE_9_0    90000
#endif  //  __IPHONE_9_0
#endif

#if defined(CRY_USE_METAL)
#include <RenderDll/XRenderD3D9/DXMETAL/CryDXMETAL.hpp>
#else
#include "XRenderD3D9/DXGL/CryDXGL.hpp"
#endif
#elif defined(CRY_USE_DX12)
#include "CryLibrary.h"
typedef uintptr_t SOCKET;
#else

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERERDEFS_H_SECTION_4
    #include AZ_RESTRICTED_FILE(RendererDefs_h)
#elif defined(WIN32) || defined(WIN64)
#include "d3d11.h"
#endif

#if defined(LEGACY_D3D9_INCLUDE)
#include "d3d9.h"
#endif

#endif
#else //defined(NULL_RENDERER)
#if defined(WIN32)
#include "windows.h"
#endif
#if (defined(LINUX) || defined(APPLE)) && !defined(DXGI_FORMAT_DEFINED)
#include <AzDXGIFormat.h>
#endif
#endif

#ifdef _DID_SKIP_WINSOCK_
#   undef _WINSOCKAPI_
#   undef _DID_SKIP_WINSOCK_
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERERDEFS_H_SECTION_5
    #include AZ_RESTRICTED_FILE(RendererDefs_h)
#endif

#if !defined(_RELEASE)
#define ENABLE_TEXTURE_STREAM_LISTENER
#endif

///////////////////////////////////////////////////////////////////////////////
/* BUFFER ACCESS
* Confetti Note -- David:
* Buffer access related defines have been moved down as some preproc. if-branches rely on CRY_USE_METAL.
*/
///////////////////////////////////////////////////////////////////////////////
// BUFFER_ENABLE_DIRECT_ACCESS
// stores pointers to actual backing storage of vertex buffers. Can only be used on architectures
// that have a unified memory architecture and further guarantee that buffer storage does not change
// on repeated accesses.
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERERDEFS_H_SECTION_10
    #include AZ_RESTRICTED_FILE(RendererDefs_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(CRY_USE_DX12) || defined(CRY_USE_METAL)
# define BUFFER_ENABLE_DIRECT_ACCESS 1
#endif

// BUFFER_USE_STAGED_UPDATES
// On platforms that support staging buffers, special buffers are allocated that act as a staging area
// for updating buffer contents on the fly.

// when staged updates are disabled CPU will have direct access to the pool's buffers' content
//  and update data directly. This cuts memory consumption and reduces the number of copies.
//  GPU won't be used to update buffer content but it will be used to perform defragmentation.
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERERDEFS_H_SECTION_11
    #include AZ_RESTRICTED_FILE(RendererDefs_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
# undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(CRY_USE_METAL)
# define BUFFER_USE_STAGED_UPDATES 0
#else
# define BUFFER_USE_STAGED_UPDATES 1
#endif

// BUFFER_SUPPORT_TRANSIENT_POOLS
// On d3d11 we want to separate the fire-and-forget allocations from the longer lived dynamic ones
#if !defined(NULL_RENDERER) && ((!defined(CONSOLE) && !defined(CRY_USE_DX12)) || defined(CRY_USE_METAL))
# define BUFFER_SUPPORT_TRANSIENT_POOLS 1
#else
# define BUFFER_SUPPORT_TRANSIENT_POOLS 0
#endif

#ifndef BUFFER_ENABLE_DIRECT_ACCESS
# define BUFFER_ENABLE_DIRECT_ACCESS 0
#endif

// CONSTANT_BUFFER_ENABLE_DIRECT_ACCESS
// Enable if we have direct access to video memory and the device manager
// should manage constant buffers
#if BUFFER_ENABLE_DIRECT_ACCESS == 1
#   if RENDERERDEFS_H_TRAIT_CONSTANT_BUFFER_ENABLE_DIRECT_ACCESS || defined (CRY_USE_DX12)
#       define CONSTANT_BUFFER_ENABLE_DIRECT_ACCESS 1
#   else
#       define CONSTANT_BUFFER_ENABLE_DIRECT_ACCESS 0
#   endif
#else
#   define CONSTANT_BUFFER_ENABLE_DIRECT_ACCESS 0
#endif

#if BUFFER_ENABLE_DIRECT_ACCESS && (defined(WIN32) || defined(WIN64)) && !defined(CRY_USE_DX12)
# error BUFFER_ENABLE_DIRECT_ACCESS is not supported on windows platforms
#endif

#if defined(WIN32)
# define FEATURE_SILHOUETTE_POM
#elif defined(WIN64)
# define FEATURE_SILHOUETTE_POM
#elif defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERERDEFS_H_SECTION_12
    #include AZ_RESTRICTED_FILE(RendererDefs_h)
#elif defined(LINUX)
# define FEATURE_SILHOUETTE_POM
#elif defined(APPLE)
# define FEATURE_SILHOUETTE_POM
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERERDEFS_H_SECTION_13
    #include AZ_RESTRICTED_FILE(RendererDefs_h)
#endif

#if !defined(_RELEASE) && !defined(NULL_RENDERER) && RENDERERDEFS_H_TRAIT_SUPPORT_D3D_DEBUG_RUNTIME && !defined(OPENGL)
#   define SUPPORT_D3D_DEBUG_RUNTIME
#endif

#if defined(NULL_RENDERER)
#   define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERERDEFS_H_SECTION_14
    #include AZ_RESTRICTED_FILE(RendererDefs_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#   undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
#   define SUPPORT_DEVICE_INFO
#   if defined(WIN32) || defined(WIN64)
#       define SUPPORT_DEVICE_INFO_MSG_PROCESSING
#       define SUPPORT_DEVICE_INFO_USER_DISPLAY_OVERRIDES
#   endif
#endif

#include <I3DEngine.h>

#if !defined(NULL_RENDERER)
#   if defined(CRY_USE_DX12)
#       include "XRenderD3D9/DX12/CryDX12.hpp"
#   elif defined(DEVICE_SUPPORTS_D3D11_1)
#       if defined(AZ_RESTRICTED_PLATFORM)
#           define AZ_RESTRICTED_SECTION RENDERERDEFS_H_SECTION_15
#           include AZ_RESTRICTED_FILE(RendererDefs_h)
#       endif
#       if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#           undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#       else
            typedef IDXGIFactory1           DXGIFactory;
            typedef IDXGIDevice1            DXGIDevice;
            typedef IDXGIAdapter1           DXGIAdapter;
            typedef IDXGIOutput             DXGIOutput;
            typedef IDXGISwapChain          DXGISwapChain;
            typedef ID3D11DeviceContextX    D3DDeviceContext;
            typedef ID3D11DeviceX           D3DDevice;
#       endif
#   else
        typedef IDXGIFactory1           DXGIFactory;
#       if !defined(ANDROID) && !defined(APPLE) && !defined(LINUX) && !defined(SKIP_TYPEDEF_OF_IDXGIDEVICE1)
            typedef IDXGIDevice1        DXGIDevice;
#       endif
        typedef IDXGIAdapter1           DXGIAdapter;
        typedef IDXGIOutput             DXGIOutput;
        typedef IDXGISwapChain          DXGISwapChain;
        typedef ID3D11DeviceContext     D3DDeviceContext;
        typedef ID3D11Device            D3DDevice;
#   endif

typedef ID3D11InputLayout       D3DVertexDeclaration;
typedef ID3D11VertexShader      D3DVertexShader;
typedef ID3D11PixelShader       D3DPixelShader;
typedef ID3D11Resource          D3DResource;

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERERDEFS_H_SECTION_16
    #include AZ_RESTRICTED_FILE(RendererDefs_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
typedef ID3D11Resource          D3DBaseTexture;
#endif

typedef ID3D11Texture2D         D3DTexture;
typedef ID3D11Texture3D         D3DVolumeTexture;
typedef ID3D11Texture2D         D3DCubeTexture;
typedef ID3D11Buffer            D3DBuffer;
typedef ID3D11ShaderResourceView    D3DShaderResourceView;
typedef ID3D11UnorderedAccessView   D3DUnorderedAccessView;
typedef ID3D11RenderTargetView  D3DSurface;
typedef ID3D11DepthStencilView  D3DDepthSurface;
typedef ID3D11View              D3DBaseView;
typedef ID3D11Query             D3DQuery;
typedef D3D11_VIEWPORT          D3DViewPort;
typedef D3D11_RECT              D3DRectangle;
typedef DXGI_FORMAT             D3DFormat;
typedef D3D11_PRIMITIVE_TOPOLOGY    D3DPrimitiveType;
typedef ID3D10Blob              D3DBlob;
typedef ID3D11SamplerState      D3DSamplerState;

#if !defined(USE_D3DX)

// this should be moved into seperate D3DX defintion file which should still be used
// for console builds, until everything in the engine has been cleaned up which still
// references this

#if defined(APPLE)
//interface is also a objective c keyword
typedef struct ID3DXConstTable       ID3DXConstTable;
typedef struct ID3DXConstTable*  LPD3DXCONSTANTTABLE;
#else
typedef interface ID3DXConstTable       ID3DXConstTable;
typedef interface ID3DXConstTable*  LPD3DXCONSTANTTABLE;
#endif



#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERERDEFS_H_SECTION_19
    #include AZ_RESTRICTED_FILE(RendererDefs_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else

#if RENDERERDEFS_H_TRAIT_DEFINE_D3DPOOL || defined(CRY_USE_DX12)
// D3DPOOL define still used as function parameters, so defined to backwards compatible with D3D9
typedef enum _D3DPOOL
{
    D3DPOOL_DEFAULT = 0,
    D3DPOOL_MANAGED = 1,
    D3DPOOL_SYSTEMMEM = 2,
    D3DPOOL_SCRATCH = 3,
    D3DPOOL_FORCE_DWORD = 0x7fffffff
} D3DPOOL;
#endif

#endif

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                                                \
    ((unsigned int)(unsigned char)(ch0) | ((unsigned int)(unsigned char)(ch1) << 8) | \
     ((unsigned int)(unsigned char)(ch2) << 16) | ((unsigned int)(unsigned char)(ch3) << 24))
#endif // defined(MAKEFOURCC)

#endif // !defined(WIN32) && !defined(WIN64)

const int32 g_nD3D10MaxSupportedSubres = (6 * 15);
//////////////////////////////////////////////////////////////////////////
#else
typedef void D3DTexture;
typedef void D3DSurface;
typedef void D3DShaderResourceView;
typedef void D3DUnorderedAccessView;
typedef void D3DDepthSurface;
typedef void D3DSamplerState;
typedef int D3DFormat;
typedef void D3DBuffer;
#endif // NULL_RENDERER
//////////////////////////////////////////////////////////////////////////

#define USAGE_WRITEONLY 8

//////////////////////////////////////////////////////////////////////////
// Linux specific defines for Renderer.
//////////////////////////////////////////////////////////////////////////

#if defined(_AMD64_) && !defined(LINUX)
#include <io.h>
#endif

#define SIZEOF_ARRAY(arr) (sizeof(arr) / sizeof((arr)[0]))

#ifdef DEBUGALLOC

#include <crtdbg.h>
#define DEBUG_CLIENTBLOCK new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_CLIENTBLOCK

// memman
#define   calloc(s, t)       _calloc_dbg(s, t, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   malloc(s)         _malloc_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   realloc(p, s)     _realloc_dbg(p, s, _NORMAL_BLOCK, __FILE__, __LINE__)

#endif

#include <CryName.h>
#include <RenderDll/Common/CryNameR.h>

#define MAX_TMU 32
#define MAX_STREAMS 16

//! Include main interfaces.
#include <IProcess.h>
#include <ITimer.h>
#include <ISystem.h>
#include <ILog.h>
#include <IConsole.h>
#include <IRenderer.h>
#include <IStreamEngine.h>
#include <CrySizer.h>
#include <smartptr.h>
#include <CryArray.h>
#include <PoolAllocator.h>

#include <CryArray.h>

enum eRenderPrimitiveType : int8
{
#if defined(NULL_RENDERER)
    eptUnknown = -1,
    eptTriangleList,
    eptTriangleStrip,
    eptLineList,
    eptLineStrip,
    eptPointList,
#else
    eptUnknown = -1,
    eptTriangleList = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
    eptTriangleStrip = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
    eptLineList = D3D11_PRIMITIVE_TOPOLOGY_LINELIST,
    eptLineStrip = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,
    eptPointList = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,
    ept1ControlPointPatchList = D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST,
    ept2ControlPointPatchList = D3D11_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST,
    ept3ControlPointPatchList = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST,
    ept4ControlPointPatchList = D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST,
#endif

    // non-real primitives, used for logical batching
    eptHWSkinGroups = 0x3f
};

inline eRenderPrimitiveType GetInternalPrimitiveType(PublicRenderPrimitiveType t)
{
    switch (t)
    {
    case prtTriangleList:
    default:
        return eptTriangleList;
    case prtTriangleStrip:
        return eptTriangleStrip;
    case prtLineList:
        return eptLineList;
    case prtLineStrip:
        return eptLineStrip;
    }
}

#if !defined(NULL_RENDERER)
#   if defined(WIN32)
#       define SUPPORT_FLEXIBLE_INDEXBUFFER // supports 16 as well as 32 bit indices AND index buffer bind offset
#   elif defined(WIN64)
#       define SUPPORT_FLEXIBLE_INDEXBUFFER // supports 16 as well as 32 bit indices AND index buffer bind offset
#   elif defined(AZ_RESTRICTED_PLATFORM)
#       define AZ_RESTRICTED_SECTION RENDERERDEFS_H_SECTION_17
#       include AZ_RESTRICTED_FILE(RendererDefs_h)
#   elif defined(LINUX)
#       define SUPPORT_FLEXIBLE_INDEXBUFFER // supports 16 as well as 32 bit indices AND index buffer bind offset
#   elif defined(APPLE)
#       define SUPPORT_FLEXIBLE_INDEXBUFFER // supports 16 as well as 32 bit indices AND index buffer bind offset
#   endif
#endif

enum RenderIndexType : int
{
#if defined(SUPPORT_FLEXIBLE_INDEXBUFFER)
    Index16 = DXGI_FORMAT_R16_UINT,
    Index32 = DXGI_FORMAT_R32_UINT
#else
    Index16,
    Index32
#endif
};

// Interfaces from the Game
extern ILog* iLog;
extern IConsole* iConsole;
extern ITimer* iTimer;
extern ISystem* iSystem;



#if defined(WIN32) || defined(WIN64) || defined(LINUX) || defined(APPLE)
#   define VOLUMETRIC_FOG_SHADOWS
#endif

#if ((defined(AZ_PLATFORM_WINDOWS) && !defined(OPENGL)) || defined(AZ_PLATFORM_MAC)) && !defined(CRY_USE_DX12) && !defined(RELEASE)
#   define ENABLE_NULL_D3D11DEVICE
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERERDEFS_H_SECTION_18
    #include AZ_RESTRICTED_FILE(RendererDefs_h)
#endif


// Enable to eliminate DevTextureDataSize calls during stream updates - costs 4 bytes per mip header
#define TEXSTRM_STORE_DEVSIZES

#ifndef TEXSTRM_BYTECENTRIC_MEMORY
#define TEXSTRM_TEXTURECENTRIC_MEMORY
#endif

#if !defined(CONSOLE) && !defined(NULL_RENDERER) && !defined(OPENGL) && !defined(CRY_INTEGRATE_DX12)
#define TEXSTRM_DEFERRED_UPLOAD
#endif

#if !defined(CONSOLE)
#define TEXSTRM_COMMIT_COOLDOWN
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERERDEFS_H_SECTION_20
    #include AZ_RESTRICTED_FILE(RendererDefs_h)
#endif

#if defined(_RELEASE)
#   define EXCLUDE_RARELY_USED_R_STATS
#endif

#include <RenderDll/Common/DevBuffer.h>
#include <RenderDll/XRenderD3D9/DeviceManager/DeviceManager.h>
