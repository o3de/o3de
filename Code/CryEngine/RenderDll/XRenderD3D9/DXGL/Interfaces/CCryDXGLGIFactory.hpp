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

// Description : Declaration of the DXGL wrapper for IDXGIFactory


#ifndef __CRYDXGLGIFACTORY__
#define __CRYDXGLGIFACTORY__

#include "CCryDXGLGIObject.hpp"

class CCryDXGLGIAdapter;

class CCryDXGLGIFactory
    : public CCryDXGLGIObject
{
public:
#if DXGL_FULL_EMULATION
    DXGL_IMPLEMENT_INTERFACE(CCryDXGLGIFactory, DXGIFactory)
#endif //DXGL_FULL_EMULATION
    DXGL_IMPLEMENT_INTERFACE(CCryDXGLGIFactory, DXGIFactory1)

    CCryDXGLGIFactory();
    ~CCryDXGLGIFactory();
    bool Initialize();

    // IDXGIFactory implementation
    HRESULT STDMETHODCALLTYPE EnumAdapters(UINT Adapter, IDXGIAdapter** ppAdapter);
    HRESULT STDMETHODCALLTYPE MakeWindowAssociation(HWND WindowHandle, UINT Flags);
    HRESULT STDMETHODCALLTYPE GetWindowAssociation(HWND* pWindowHandle);
    HRESULT STDMETHODCALLTYPE CreateSwapChain(IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain);
    HRESULT STDMETHODCALLTYPE CreateSoftwareAdapter(HMODULE Module, IDXGIAdapter** ppAdapter);

    // IDXGIFactory1 implementation
    HRESULT STDMETHODCALLTYPE EnumAdapters1(UINT Adapter, IDXGIAdapter1** ppAdapter);
    BOOL STDMETHODCALLTYPE IsCurrent();
protected:
    typedef std::vector<_smart_ptr<CCryDXGLGIAdapter> > Adapters;
protected:
    // The adapters available on this system
    Adapters m_kAdapters;
    HWND m_hWindowHandle;
};

#endif //__CRYDXGLGIFACTORY__
