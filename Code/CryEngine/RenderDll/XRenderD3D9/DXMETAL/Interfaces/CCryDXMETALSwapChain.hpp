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

// Description : Declaration of the DXGL wrapper for IDXGISwapChain

#ifndef __CRYMETALGLSWAPCHAIN__
#define __CRYMETALGLSWAPCHAIN__

#include "CCryDXMETALBase.hpp"
#include "CCryDXMETALGIObject.hpp"

@protocol CAMetalDrawable;


namespace NCryMetal
{
    class CDeviceContextProxy;
}

class CCryDXGLDevice;
class CCryDXGLTexture2D;

@class MetalView;

class CCryDXGLSwapChain
    : public CCryDXGLGIObject
{
public:
#if DXGL_FULL_EMULATION
    DXGL_IMPLEMENT_INTERFACE(CCryDXGLSwapChain, DXGIDeviceSubObject)
#endif //DXGL_FULL_EMULATION
    DXGL_IMPLEMENT_INTERFACE(CCryDXGLSwapChain, DXGISwapChain)

    CCryDXGLSwapChain(CCryDXGLDevice* pDevice, const DXGI_SWAP_CHAIN_DESC& kDesc);
    ~CCryDXGLSwapChain();

    bool Initialize();

    // IDXGISwapChain implementation
    HRESULT STDMETHODCALLTYPE Present(UINT SyncInterval, UINT Flags);
    HRESULT STDMETHODCALLTYPE GetBuffer(UINT Buffer, REFIID riid, void** ppSurface);
    HRESULT STDMETHODCALLTYPE SetFullscreenState(BOOL Fullscreen, IDXGIOutput* pTarget);
    HRESULT STDMETHODCALLTYPE GetFullscreenState(BOOL* pFullscreen, IDXGIOutput** ppTarget);
    HRESULT STDMETHODCALLTYPE GetDesc(DXGI_SWAP_CHAIN_DESC* pDesc);
    HRESULT STDMETHODCALLTYPE ResizeBuffers(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
    HRESULT STDMETHODCALLTYPE ResizeTarget(const DXGI_MODE_DESC* pNewTargetParameters);
    HRESULT STDMETHODCALLTYPE GetContainingOutput(IDXGIOutput** ppOutput);
    HRESULT STDMETHODCALLTYPE GetFrameStatistics(DXGI_FRAME_STATISTICS* pStats);
    HRESULT STDMETHODCALLTYPE GetLastPresentCount(UINT* pLastPresentCount);

    // IDXGIDeviceSubObject implementation
    HRESULT STDMETHODCALLTYPE GetDevice(REFIID riid, void** ppDevice) { return E_NOTIMPL; }

    void    TryCreateAutoreleasePool();
    void    FlushAutoreleasePool();

protected:
    bool CreateDrawableView();
    bool UpdateTexture(bool bSetPixelFormat);
protected:
    _smart_ptr<CCryDXGLDevice> m_spDevice;
    _smart_ptr<CCryDXGLTexture2D> m_spBackBufferTexture;
    _smart_ptr<CCryDXGLTexture2D> m_spExposedBackBufferTexture;
    DXGI_SWAP_CHAIN_DESC m_kDesc;

    MetalView* m_currentView;

    id<CAMetalDrawable> m_Drawable;

    void* m_pAutoreleasePool;
};

#endif //__CRYMETALGLSWAPCHAIN__