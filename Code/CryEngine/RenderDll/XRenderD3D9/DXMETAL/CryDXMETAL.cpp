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

// Description : Contains the definitions of the global functions defined in
//               CryDXGL

#include "RenderDll_precompiled.h"
#include "Interfaces/CCryDXMETALDevice.hpp"
#include "Interfaces/CCryDXMETALBlob.hpp"
#include "Interfaces/CCryDXMETALDevice.hpp"
#include "Interfaces/CCryDXMETALDeviceContext.hpp"
#include "Interfaces/CCryDXMETALDeviceContext.hpp"
#include "Interfaces/CCryDXMETALGIFactory.hpp"
#include "Implementation/GLShader.hpp"
#include "Implementation/MetalDevice.hpp"

//  Confetti BEGIN: Igor Lobanchikov
#include "Interfaces/CCryDXMETALDeviceContext.hpp"
#include "XRenderD3D9/DriverD3D.h"
#include "Implementation/METALContext.hpp"
//  Confetti End: Igor Lobanchikov

#if defined(CRY_DXCAPTURE_ENABLED)
#include "Debug/DXCapture.h"
#endif //defined(CRY_DXCAPTURE_ENABLED)

template <typename Factory>
HRESULT CreateDXGIFactoryInternal(REFIID riid, void** ppFactory)
{
    if (riid == __uuidof(Factory))
    {
        CCryDXGLGIFactory* pFactory(new CCryDXGLGIFactory());
        if (!pFactory->Initialize())
        {
            pFactory->Release();
            *ppFactory = NULL;
            return E_FAIL;
        }
#if defined(CRY_DXCAPTURE_ENABLED)
        *ppFactory = DXCapture::GetWrapper(pFactory);
        pFactory->Release();
#else
        CCryDXGLGIFactory::ToInterface(reinterpret_cast<IDXGIFactory**>(ppFactory), pFactory);
#endif
        return S_OK;
    }

    return E_FAIL;
}


////////////////////////////////////////////////////////////////////////////
//  Required global functions declared in D3D11.h and included headers
////////////////////////////////////////////////////////////////////////////

DXGL_API HRESULT WINAPI D3D11CreateDeviceAndSwapChain(
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
    ID3D11DeviceContext** ppImmediateContext)
{
    if (pAdapter == NULL)
    {
        // Get the first adapter enumerated by the factory according to the specification
        void* pvFactory(NULL);
        HRESULT iResult = CreateDXGIFactoryInternal<IDXGIFactory1>(__uuidof(IDXGIFactory1), &pvFactory);
        if (FAILED(iResult))
        {
            return iResult;
        }
        CCryDXGLGIFactory* pFactory(CCryDXGLGIFactory::FromInterface(static_cast<IDXGIFactory1*>(pvFactory)));
        iResult = pFactory->EnumAdapters(0, &pAdapter);
        pFactory->Release();
        if (FAILED(iResult))
        {
            return iResult;
        }
    }

    CRY_ASSERT(pAdapter != NULL);
    CCryDXGLGIAdapter* pDXGLAdapter(CCryDXGLGIAdapter::FromInterface(pAdapter));

    D3D_FEATURE_LEVEL eDevFeatureLevel;
    if (pDXGLAdapter != NULL)
    {
        eDevFeatureLevel = pDXGLAdapter->GetSupportedFeatureLevel();
    }
    else
    {
        DXGL_TODO("Get the supported feature level even if no adapter is specified")
        eDevFeatureLevel = D3D_FEATURE_LEVEL_11_0;
    }

    if (pFeatureLevels != NULL && FeatureLevels > 0)
    {
        D3D_FEATURE_LEVEL uMaxAllowedFL(*pFeatureLevels);
        while (FeatureLevels > 1)
        {
            ++pFeatureLevels;
            uMaxAllowedFL = max(uMaxAllowedFL, *pFeatureLevels);
            --FeatureLevels;
        }
        eDevFeatureLevel = min(eDevFeatureLevel, uMaxAllowedFL);
    }

    if (pFeatureLevel != NULL)
    {
        *pFeatureLevel = eDevFeatureLevel;
    }

    if (ppDevice != NULL)
    {
        _smart_ptr<CCryDXGLDevice> spDevice(new CCryDXGLDevice(pDXGLAdapter, eDevFeatureLevel));
        if (!spDevice->Initialize(pSwapChainDesc, ppSwapChain))
        {
            return E_FAIL;
        }

#if defined(CRY_DXCAPTURE_ENABLED)
        if (ppSwapChain != NULL)
        {
            *ppSwapChain = DXCapture::GetWrapper(CCryDXGLSwapChain::FromInterface(*ppSwapChain));
        }
        *ppDevice = DXCapture::GetWrapper(spDevice.get());
        spDevice->Release();
#else
        CCryDXGLDevice::ToInterface(ppDevice, spDevice);
#endif

        if (ppImmediateContext != NULL)
        {
            (*ppDevice)->GetImmediateContext(ppImmediateContext);
        }
    }

    return S_OK;
}

DXGL_API HRESULT WINAPI D3D10CreateBlob(SIZE_T NumBytes, LPD3D10BLOB* ppBuffer)
{
    CCryDXGLBlob::ToInterface(ppBuffer, new CCryDXGLBlob(NumBytes));
    return (*ppBuffer)->GetBufferPointer() != NULL;
}


////////////////////////////////////////////////////////////////////////////
//  Required global functions declared in D3DCompiler.h and included headers
////////////////////////////////////////////////////////////////////////////

DXGL_API HRESULT WINAPI D3DReflect(
    LPCVOID pSrcData,
    SIZE_T SrcDataSize,
    REFIID pInterface,
    void** ppReflector)
{
    if (pInterface == IID_ID3D11ShaderReflection)
    {
        CCryDXGLShaderReflection* pReflection(new CCryDXGLShaderReflection());

        const void* pvData(pSrcData);

        if (pReflection->Initialize(pvData))
        {
            CCryDXGLShaderReflection::ToInterface(reinterpret_cast<ID3D11ShaderReflection**>(ppReflector), pReflection);
            return S_OK;
        }
        pReflection->Release();
    }
    return E_FAIL;
}

DXGL_API HRESULT WINAPI D3DDisassemble(
    LPCVOID pSrcData,
    SIZE_T SrcDataSize,
    UINT Flags,
    LPCSTR szComments,
    ID3DBlob** ppDisassembly)
{
    DXGL_NOT_IMPLEMENTED
    return E_FAIL;
}


////////////////////////////////////////////////////////////////////////////
//  Required global functions declared in D3DX11.h and included headers
////////////////////////////////////////////////////////////////////////////

DXGL_API HRESULT WINAPI D3DX11CreateTextureFromMemory(
    ID3D11Device* pDevice,
    const void* pSrcData,
    size_t SrcDataSize,
    D3DX11_IMAGE_LOAD_INFO* pLoadInfo,
    ID3DX11ThreadPump* pPump,
    ID3D11Resource** ppTexture,
    long* pResult)
{
    DXGL_NOT_IMPLEMENTED
    return E_FAIL;
}

DXGL_API HRESULT WINAPI D3DX11SaveTextureToFile(ID3D11DeviceContext* pDevice,
    ID3D11Resource* pSrcResource,
    D3DX11_IMAGE_FILE_FORMAT fmt,
    const char* pDestFile)
{
    DXGL_NOT_IMPLEMENTED
    return E_FAIL;
}

DXGL_API HRESULT WINAPI D3DX11CompileFromMemory(
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
    HRESULT* pHResult)
{
    DXGL_NOT_IMPLEMENTED
    return E_FAIL;
}


////////////////////////////////////////////////////////////////////////////
//  Required global functions declared in dxgi.h and included headers
////////////////////////////////////////////////////////////////////////////

DXGL_API HRESULT WINAPI CreateDXGIFactory(REFIID riid, void** ppFactory)
{
    return CreateDXGIFactoryInternal<IDXGIFactory>(riid, ppFactory);
}

DXGL_API HRESULT WINAPI CreateDXGIFactory1(REFIID riid, void** ppFactory)
{
    return CreateDXGIFactoryInternal<IDXGIFactory1>(riid, ppFactory);
}


////////////////////////////////////////////////////////////////////////////
//  Frame debugging functions
////////////////////////////////////////////////////////////////////////////

#define DXGL_PROFILE_USE_GREMEDY_STRING_MARKER 0
#define DXGL_PROFILE_USE_KHR_DEBUG 1
#define DXGL_PROFILE_USE_NVTX 0

#if DXGL_PROFILE_USE_GREMEDY_STRING_MARKER

template <size_t uSuffixSize>
struct SDebugStringBuffer
{
    enum
    {
        MAX_TEXT_LENGTH = 1024
    };
    char s_acBuffer[MAX_TEXT_LENGTH + uSuffixSize];

    SDebugStringBuffer(const char* szSuffix)
    {
        cryMemcpy(s_acBuffer + MAX_TEXT_LENGTH, szSuffix, uSuffixSize);
    }

    const char* Write(const char* szText)
    {
        size_t uTextLength(min((size_t)MAX_TEXT_LENGTH, strlen(szText)));
        char* szDest(s_acBuffer + MAX_TEXT_LENGTH - uTextLength);
        cryMemcpy(szDest, szText, uTextLength);
        return szDest;
    }
};

#define DXGL_DEBUG_STRING_BUFFER(_Suffix, _Name) SDebugStringBuffer<DXGL_ARRAY_SIZE(_Suffix)> _Name(_Suffix)

DXGL_DEBUG_STRING_BUFFER(": enter", g_kEnterDebugBuffer);
DXGL_DEBUG_STRING_BUFFER(": leave", g_kLeaveDebugBuffer);

#endif //DXGL_PROFILE_USE_GREMEDY_STRING_MARKER

#if DXGL_PROFILE_USE_NVTX
#include <nvToolsExt.h>
    #if defined(WIN32)
        #pragma comment(lib, "nvToolsExt32_1.lib")
    #elif defined(WIN64)
        #pragma comment(lib, "nvToolsExt64_1.lib")
    #endif
#endif //DXGL_PROFILE_USE_NVTX

DXGL_API void DXGLProfileLabel(const char* szName)
{
    //  Confetti BEGIN: Igor Lobanchikov
    NCryMetal::CContext* pContext = CCryDXGLDeviceContext::FromInterface(((CD3D9Renderer*)gEnv->pRenderer)->DevInfo().Context())->GetMetalContext();
    pContext->ProfileLabel(szName);
    //  Confetti End: Igor Lobanchikov
}

DXGL_API void DXGLProfileLabelPush(const char* szName)
{
    //  Confetti BEGIN: Igor Lobanchikov
    NCryMetal::CContext* pContext = CCryDXGLDeviceContext::FromInterface(((CD3D9Renderer*)gEnv->pRenderer)->DevInfo().Context())->GetMetalContext();
    pContext->ProfileLabelPush(szName);
    //  Confetti End: Igor Lobanchikov
}

DXGL_API void DXGLProfileLabelPop(const char* szName)
{
    //  Confetti BEGIN: Igor Lobanchikov
    NCryMetal::CContext* pContext = CCryDXGLDeviceContext::FromInterface(((CD3D9Renderer*)gEnv->pRenderer)->DevInfo().Context())->GetMetalContext();
    pContext->ProfileLabelPop(szName);
    //  Confetti End: Igor Lobanchikov
}

inline CCryDXGLDevice* GetDXGLDevice(ID3D11Device* pDevice)
{
#if defined(CRY_DXCAPTURE_ENABLED)
    return static_cast<CCryDXGLDevice*>(DXCapture::GetWrapped(pDevice));
#else
    return CCryDXGLDevice::FromInterface(pDevice);
#endif
}

inline CCryDXGLDeviceContext* GetDXGLDeviceContext(ID3D11DeviceContext* pDeviceContext)
{
#if defined(CRY_DXCAPTURE_ENABLED)
    return static_cast<CCryDXGLDeviceContext*>(DXCapture::GetWrapped(pDeviceContext));
#else
    return CCryDXGLDeviceContext::FromInterface(pDeviceContext);
#endif
}

////////////////////////////////////////////////////////////////////////////
//  DXMetal Extensions
////////////////////////////////////////////////////////////////////////////

void* DXMETALGetBufferStorage(ID3D11Buffer* pBuffer)
{
    NCryMetal::SBuffer* pGLBuffer(pBuffer->GetGLBuffer());

    if (pGLBuffer->m_eUsage == NCryMetal::eBU_DirectAccess)
    {
        //DirectAccess buffers allocate from shared memory only.
        CRY_ASSERT(pGLBuffer->m_BufferShared);
        return pGLBuffer->m_BufferShared.contents;
    }

    return NULL;
}

////////////////////////////////////////////////////////////////////////////
//  DXMetal Extensions
////////////////////////////////////////////////////////////////////////////

void DXMETALSetColorDontCareActions(ID3D11RenderTargetView* const rtv,
    bool const loadDontCare,
    bool const storeDontCare)
{
    CRY_ASSERT(NULL != rtv);

    NCryMetal::SOutputMergerView* somv = rtv->GetGLView();
    CRY_ASSERT(somv);

    NCryMetal::SOutputMergerTextureView* somtv = somv->AsSOutputMergerTextureView();
    CRY_ASSERT(somtv);

    NCryMetal::STexture* tex = somtv->m_pTexture;
    CRY_ASSERT(tex);

    if (loadDontCare)
    {
        CRY_ASSERT(!tex->m_spTextureViewToClear);
        if (tex->m_spTextureViewToClear)
        {
            DXGL_ERROR("Can't set MTLLoadActionDontCare if the resource is already set to be cleared.");
        }
    }

    tex->m_bColorLoadDontCare   = loadDontCare;
    tex->m_bColorStoreDontCare  = storeDontCare;
}

void DXMETALSetDepthDontCareActions(ID3D11DepthStencilView* const dsv,
    bool const loadDontCare,
    bool const storeDontCare)
{
    CRY_ASSERT(NULL != dsv);

    NCryMetal::SOutputMergerView* somv = dsv->GetGLView();
    CRY_ASSERT(somv);

    NCryMetal::SOutputMergerTextureView* somtv = somv->AsSOutputMergerTextureView();
    CRY_ASSERT(somtv);

    NCryMetal::STexture* tex = somtv->m_pTexture;
    CRY_ASSERT(tex);

    if (loadDontCare)
    {
        CRY_ASSERT(!tex->m_spTextureViewToClear);
        if (tex->m_spTextureViewToClear)
        {
            DXGL_ERROR("Can't set MTLLoadActionDontCare if the resource is already set to be cleared.");
        }
    }

    tex->m_bDepthLoadDontCare   = loadDontCare;
    tex->m_bDepthStoreDontCare  = storeDontCare;
}

void DXMETALSetStencilDontCareActions(ID3D11DepthStencilView* const dsv,
    bool const loadDontCare,
    bool const storeDontCare)
{
    CRY_ASSERT(NULL != dsv);

    NCryMetal::SOutputMergerView* somv = dsv->GetGLView();
    CRY_ASSERT(somv);

    NCryMetal::SOutputMergerTextureView* somtv = somv->AsSOutputMergerTextureView();
    CRY_ASSERT(somtv);

    NCryMetal::STexture* tex = somtv->m_pTexture;
    CRY_ASSERT(tex);

    if (loadDontCare)
    {
        CRY_ASSERT(!tex->m_spStencilTextureViewToClear);
        if (tex->m_spStencilTextureViewToClear)
        {
            DXGL_ERROR("Can't set MTLLoadActionDontCare if the resource is already set to be cleared.");
        }
    }

    tex->m_bStencilLoadDontCare   = loadDontCare;
    tex->m_bStencilStoreDontCare  = storeDontCare;
}

////////////////////////////////////////////////////////////////////////////
//  DXGL Extensions
////////////////////////////////////////////////////////////////////////////

#if !CRY_DXGL_FULL_EMULATION
HRESULT DXGLMapBufferRange(ID3D11DeviceContext* pDeviceContext, ID3D11Buffer* pBuffer, size_t uOffset, size_t uSize, D3D11_MAP MapType, UINT MapFlags, D3D11_MAPPED_SUBRESOURCE* pMappedResource)
{
    NCryMetal::CContext* pGLContext(static_cast<CCryDXGLDeviceContext*>(pDeviceContext)->GetMetalContext());
    NCryMetal::SBuffer* pGLBuffer(pBuffer->GetGLBuffer());
    return (*pGLBuffer->m_pfMapBufferRange)(pGLBuffer, uOffset, uSize, MapType, MapFlags, pMappedResource, pGLContext) ? S_OK : E_FAIL;
}

void DXGLSetDepthBoundsTest(bool bEnabled, float fMin, float fMax)
{
#if defined(GL_EXT_depth_bounds_test)
    if (bEnabled)
    {
        glEnable(GL_DEPTH_BOUNDS_TEST_EXT);
    }
    else
    {
        glDisable(GL_DEPTH_BOUNDS_TEST_EXT);
    }
    glDepthBoundsEXT(fMin, fMax);
#else
    DXGL_WARNING("Depths Bounds Test extension not available on this platform");
#endif
}

#endif //!DXGL_FULL_EMULATION

DXGL_API bool DXGLCreateMetalWindow(
    const char* szTitle,
    uint32 width,
    uint32 height,
    bool fullScreen,
    HWND* pHandle)
{
    return NCryMetal::CDevice::CreateMetalWindow(pHandle, width, height, fullScreen);
}

DXGL_EXTERN DXGL_API void DXGLDestroyMetalWindow(HWND kHandle)
{
    return NCryMetal::CDevice::DestroyMetalWindow(kHandle);
}

////////////////////////////////////////////////////////////////////////////
//  DxErr Logging and error functions
////////////////////////////////////////////////////////////////////////////

DXGL_API const char*  WINAPI DXGetErrorStringA(HRESULT hr)
{
    DXGL_NOT_IMPLEMENTED
    return "";
}

DXGL_API const WCHAR* WINAPI DXGetErrorStringW(HRESULT hr)
{
    DXGL_NOT_IMPLEMENTED
    return L"";
}

DXGL_API const char*  WINAPI DXGetErrorDescriptionA(HRESULT hr)
{
    DXGL_NOT_IMPLEMENTED
    return "";
}

DXGL_API const WCHAR* WINAPI DXGetErrorDescriptionW(HRESULT hr)
{
    DXGL_NOT_IMPLEMENTED
    return L"";
}

DXGL_API HRESULT WINAPI DXTraceA(const char* strFile, DWORD dwLine, HRESULT hr, const char* strMsg, BOOL bPopMsgBox)
{
    DXGL_NOT_IMPLEMENTED
    return E_FAIL;
}

DXGL_API HRESULT WINAPI DXTraceW(const char* strFile, DWORD dwLine, HRESULT hr, const WCHAR* strMsg, BOOL bPopMsgBox)
{
    DXGL_NOT_IMPLEMENTED
    return E_FAIL;
}
