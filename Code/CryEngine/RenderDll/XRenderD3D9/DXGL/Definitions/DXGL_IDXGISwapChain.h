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

// Description : Contains a definition of the IDXGISwapChain interface
//               matching the one in the DirectX SDK


#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERD3D9_DXGL_DEFINITIONS_DXGL_IDXGISWAPCHAIN_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERD3D9_DXGL_DEFINITIONS_DXGL_IDXGISWAPCHAIN_H
#pragma once


#if !DXGL_FULL_EMULATION
#include "../Interfaces/CCryDXGLGIObject.hpp"
#endif //!DXGL_FULL_EMULATION

#if DXGL_FULL_EMULATION
struct IDXGISwapChain
    : IDXGIDeviceSubObject
#else
struct IDXGISwapChain
    : CCryDXGLGIObject
#endif
{
    // IDXGISwapChain interface
    virtual HRESULT STDMETHODCALLTYPE Present(UINT SyncInterval, UINT Flags) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetBuffer(UINT Buffer, REFIID riid, void** ppSurface) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetFullscreenState(BOOL Fullscreen, IDXGIOutput* pTarget) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetFullscreenState(BOOL* pFullscreen, IDXGIOutput** ppTarget) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDesc(DXGI_SWAP_CHAIN_DESC* pDesc) = 0;
    virtual HRESULT STDMETHODCALLTYPE ResizeBuffers(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE ResizeTarget(const DXGI_MODE_DESC* pNewTargetParameters) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetContainingOutput(IDXGIOutput** ppOutput) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetFrameStatistics(DXGI_FRAME_STATISTICS* pStats) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetLastPresentCount(UINT* pLastPresentCount) = 0;

    // IDXGIDeviceSubObject interface
    virtual HRESULT STDMETHODCALLTYPE GetDevice(REFIID riid, void** ppDevice) = 0;
};

#endif // CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERD3D9_DXGL_DEFINITIONS_DXGL_IDXGISWAPCHAIN_H
