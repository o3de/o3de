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

// Description : Entry point header for the DXGL wrapper library

#ifndef __CRYMETALGL__
#define __CRYMETALGL__

#if !defined(DXGL_FULL_EMULATION)
#define DXGL_FULL_EMULATION 0
#endif //!defined(DXGL_FULL_EMULATION)

#include "Definitions/CryDXMETALMisc.hpp"

#if defined(CRY_DXCAPTURE_ENABLED) || DXGL_FULL_EMULATION
    #include "Definitions/DXMETAL_IDXGISwapChain.h"
    #include "Definitions/DXMETAL_IDXGIFactory1.h"
#endif

#if DXGL_VIRTUAL_DEVICE_AND_CONTEXT || DXGL_FULL_EMULATION
#include "Definitions/DXMETAL_ID3D11Device.h"
#include "Definitions/DXMETAL_ID3D11DeviceContext.h"
#else
#include "Interfaces/CCryDXMETALDevice.hpp"
#include "Interfaces/CCryDXMETALDeviceContext.hpp"
#endif

#if !DXGL_FULL_EMULATION
#include "Interfaces/CCryDXMETALBlob.hpp"
#include "Interfaces/CCryDXMETALSwapChain.hpp"
#include "Interfaces/CCryDXMETALQuery.hpp"
#include "Interfaces/CCryDXMETALSwitchToRef.hpp"

#include "Interfaces/CCryDXMETALBuffer.hpp"
#include "Interfaces/CCryDXMETALDepthStencilView.hpp"
#include "Interfaces/CCryDXMETALInputLayout.hpp"
#include "Interfaces/CCryDXMETALRenderTargetView.hpp"
#include "Interfaces/CCryDXMETALShader.hpp"
#include "Interfaces/CCryDXMETALDeviceContext.hpp"
#include "Interfaces/CCryDXMETALDepthStencilView.hpp"
#include "Interfaces/CCryDXMETALShaderResourceView.hpp"
#include "Interfaces/CCryDXMETALUnorderedAccessView.hpp"
#include "Interfaces/CCryDXMETALView.hpp"
#include "Interfaces/CCryDXMETALBlendState.hpp"
#include "Interfaces/CCryDXMETALDepthStencilState.hpp"
#include "Interfaces/CCryDXMETALRasterizerState.hpp"
#include "Interfaces/CCryDXMETALSamplerState.hpp"
#include "Interfaces/CCryDXMETALTexture1D.hpp"
#include "Interfaces/CCryDXMETALTexture2D.hpp"
#include "Interfaces/CCryDXMETALTexture3D.hpp"
#include "Interfaces/CCryDXMETALDevice.hpp"
#include "Interfaces/CCryDXMETALGIFactory.hpp"
#include "Interfaces/CCryDXMETALGIOutput.hpp"

#include "Interfaces/CCryDXMETALShaderReflection.hpp"
#include "Interfaces/CCryDXMETALGIAdapter.hpp"
#endif //!DXGL_FULL_EMULATION

// Metal requires that all RTs and depth buffer have the same size.
#define CRY_OPENGL_DO_NOT_ALLOW_LARGER_RT

typedef ID3D10Blob* LPD3D10BLOB;
typedef ID3D10Blob ID3DBlob;

#if DXGL_FULL_EMULATION
    #if defined(OPENGL)
        #define DXGL_API DXGL_IMPORT
    #else
        #define DXGL_API DXGL_EXPORT
    #endif
#else
    #define DXGL_API
#endif

#if DXGL_FULL_EMULATION && !defined(WIN32)
#define DXGL_EXTERN extern "C"
#else
#define DXGL_EXTERN
#endif


////////////////////////////////////////////////////////////////////////////
//  Required global functions declared in D3D11.h and included headers
////////////////////////////////////////////////////////////////////////////

typedef HRESULT (WINAPI * PFN_D3D11_CREATE_DEVICE_AND_SWAP_CHAIN)(IDXGIAdapter*,
    D3D_DRIVER_TYPE, HMODULE, UINT,
    CONST D3D_FEATURE_LEVEL*,
    UINT FeatureLevels, UINT, CONST DXGI_SWAP_CHAIN_DESC*,
    IDXGISwapChain**, ID3D11Device**,
    D3D_FEATURE_LEVEL*, ID3D11DeviceContext**);

DXGL_EXTERN DXGL_API HRESULT WINAPI D3D11CreateDeviceAndSwapChain(
    IDXGIAdapter* pAdapter,
    D3D_DRIVER_TYPE DriverType,
    HMODULE Software,
    UINT Flags,
    CONST D3D_FEATURE_LEVEL* pFeatureLevels,
    UINT FeatureLevels,
    UINT SDKVersion,
    CONST DXGI_SWAP_CHAIN_DESC* pSwapChainDesc,
    IDXGISwapChain** ppSwapChain,
    ID3D11Device** ppDevice,
    D3D_FEATURE_LEVEL* pFeatureLevel,
    ID3D11DeviceContext** ppImmediateContext);

DXGL_EXTERN DXGL_API HRESULT WINAPI D3D10CreateBlob(SIZE_T NumBytes, LPD3D10BLOB* ppBuffer);


////////////////////////////////////////////////////////////////////////////
//  Required global functions declared in D3DCompiler.h and included headers
////////////////////////////////////////////////////////////////////////////

DXGL_EXTERN DXGL_API HRESULT WINAPI D3DReflect(
    LPCVOID pSrcData,
    SIZE_T SrcDataSize,
    REFIID pInterface,
    void** ppReflector);

DXGL_EXTERN DXGL_API HRESULT WINAPI D3DDisassemble(
    LPCVOID pSrcData,
    SIZE_T SrcDataSize,
    UINT Flags,
    LPCSTR szComments,
    ID3DBlob** ppDisassembly);


////////////////////////////////////////////////////////////////////////////
//  Required global functions declared in D3DX11.h and included headers
////////////////////////////////////////////////////////////////////////////

DXGL_EXTERN DXGL_API HRESULT WINAPI D3DX11CreateTextureFromMemory(
    ID3D11Device* pDevice,
    const void* pSrcData,
    size_t SrcDataSize,
    D3DX11_IMAGE_LOAD_INFO* pLoadInfo,
    ID3DX11ThreadPump* pPump,
    ID3D11Resource** ppTexture,
    long* pResult);

DXGL_EXTERN DXGL_API HRESULT WINAPI D3DX11SaveTextureToFile(ID3D11DeviceContext* pDevice,
    ID3D11Resource* pSrcResource,
    D3DX11_IMAGE_FILE_FORMAT fmt,
    const char* pDestFile);

DXGL_EXTERN DXGL_API HRESULT WINAPI D3DX11CompileFromMemory(
    LPCSTR pSrcData,
    SIZE_T SrcDataLen,
    LPCSTR pFileName,
    CONST D3D10_SHADER_MACRO* pDefines,
    LPD3D10INCLUDE pInclude,
    LPCSTR pFunctionName,
    LPCSTR pProfile,
    UINT Flags1,
    UINT Flags2,
    ID3DX11ThreadPump* pPump,
    ID3D10Blob** ppShader,
    ID3D10Blob** ppErrorMsgs,
    HRESULT* pHResult);


////////////////////////////////////////////////////////////////////////////
//  Required global functions declared in dxgi.h and included headers
////////////////////////////////////////////////////////////////////////////

DXGL_EXTERN DXGL_API HRESULT WINAPI CreateDXGIFactory(REFIID riid, void** ppFactory);

DXGL_EXTERN DXGL_API HRESULT WINAPI CreateDXGIFactory1(REFIID riid, void** ppFactory);


////////////////////////////////////////////////////////////////////////////
//  Frame debugging functions
////////////////////////////////////////////////////////////////////////////

DXGL_EXTERN DXGL_API void DXGLProfileLabel(const char* szName);

DXGL_EXTERN DXGL_API void DXGLProfileLabelPush(const char* szName);

DXGL_EXTERN DXGL_API void DXGLProfileLabelPop(const char* szName);


////////////////////////////////////////////////////////////////////////////
//  DXMetal Extensions
////////////////////////////////////////////////////////////////////////////

void* DXMETALGetBufferStorage(ID3D11Buffer* pBuffer);

////////////////////////////////////////////////////////////////////////////
//  DXMetal Extensions
////////////////////////////////////////////////////////////////////////////

void DXMETALSetColorDontCareActions(ID3D11RenderTargetView* const rtv,
    bool const loadDontCare     = false,
    bool const storeDontCare    = false);
void DXMETALSetDepthDontCareActions(ID3D11DepthStencilView* const dsv,
    bool const loadDontCare     = false,
    bool const storeDontCare    = false);
void DXMETALSetStencilDontCareActions(ID3D11DepthStencilView* const dsv,
    bool const loadDontCare     = false,
    bool const storeDontCare    = false);
////////////////////////////////////////////////////////////////////////////
//  DXGL Extensions
////////////////////////////////////////////////////////////////////////////

#if !DXGL_FULL_EMULATION

HRESULT DXGLMapBufferRange(ID3D11DeviceContext* pDeviceContext, ID3D11Buffer* pBuffer, size_t uOffset, size_t uSize, D3D11_MAP MapType, UINT MapFlags, D3D11_MAPPED_SUBRESOURCE* pMappedResource);

void DXGLSetDepthBoundsTest(bool bEnabled, float fMin, float fMax);

#endif //!DXGL_FULL_EMULATION

DXGL_EXTERN DXGL_API bool DXGLCreateMetalWindow(const char* szTitle, uint32 uWidth, uint32 uHeight, bool bFullScreen, HWND* pHandle);
DXGL_EXTERN DXGL_API void DXGLDestroyMetalWindow(HWND kHandle);

////////////////////////////////////////////////////////////////////////////
//  DxErr Logging and error functions
////////////////////////////////////////////////////////////////////////////

DXGL_EXTERN DXGL_API const char*  WINAPI DXGetErrorStringA(HRESULT hr);
DXGL_EXTERN DXGL_API const WCHAR* WINAPI DXGetErrorStringW(HRESULT hr);

DXGL_EXTERN DXGL_API const char*  WINAPI DXGetErrorDescriptionA(HRESULT hr);
DXGL_EXTERN DXGL_API const WCHAR* WINAPI DXGetErrorDescriptionW(HRESULT hr);

DXGL_EXTERN DXGL_API HRESULT WINAPI DXTraceA(const char* strFile, DWORD dwLine, HRESULT hr, const char* strMsg, BOOL bPopMsgBox);
DXGL_EXTERN DXGL_API HRESULT WINAPI DXTraceW(const char* strFile, DWORD dwLine, HRESULT hr, const WCHAR* strMsg, BOOL bPopMsgBox);

#ifdef UNICODE
    #define DXGetErrorString DXGetErrorStringW
    #define DXGetErrorDescription DXGetErrorDescriptionW
    #define DXTrace DXTraceW
#else
    #define DXGetErrorString DXGetErrorStringA
    #define DXGetErrorDescription DXGetErrorDescriptionA
    #define DXTrace DXTraceA
#endif

#endif //__CRYMETALGL__

