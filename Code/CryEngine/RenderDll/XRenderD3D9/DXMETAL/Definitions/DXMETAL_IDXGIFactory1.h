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

// Description : Contains a definition of the IDXGIFactory1 interface
//               matching the one in the DirectX SDK

#ifndef __DXGL_IDXGIFactory1_h__
#define __DXGL_IDXGIFactory1_h__

#if !DXGL_FULL_EMULATION
#include "../Interfaces/CCryDXMETALGIObject.hpp"
#endif //!DXGL_FULL_EMULATION

struct IDXGIFactory1
#if DXGL_FULL_EMULATION
    : IDXGIFactory
#else
    : CCryDXGLGIObject
#endif
{
    // IDXGIFactory interface
    virtual HRESULT STDMETHODCALLTYPE EnumAdapters(UINT Adapter, IDXGIAdapter** ppAdapter) = 0;
    virtual HRESULT STDMETHODCALLTYPE MakeWindowAssociation(HWND WindowHandle, UINT Flags) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetWindowAssociation(HWND* pWindowHandle) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateSwapChain(IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateSoftwareAdapter(HMODULE Module, IDXGIAdapter** ppAdapter) = 0;

    // IDXGIFactory1 interface
    virtual HRESULT STDMETHODCALLTYPE EnumAdapters1(UINT Adapter, IDXGIAdapter1** ppAdapter) = 0;
    virtual BOOL STDMETHODCALLTYPE IsCurrent() = 0;
};

#endif // __DXGL_IDXGIFactory1_h__
