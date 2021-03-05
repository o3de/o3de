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
#ifndef __CCRYDX12GIFACTORY__
#define __CCRYDX12GIFACTORY__

#include "DX12/CCryDX12Object.hpp"

#if defined(AZ_RESTRICTED_PLATFORM)
    #undef AZ_RESTRICTED_SECTION
    #define CCRYDX12GIFACTORY_HPP_SECTION_1 1
#endif

class CCryDX12GIFactory
    : public CCryDX12GIObject<IDXGIFactory1>
{
public:
    DX12_OBJECT(CCryDX12GIFactory, CCryDX12GIObject<IDXGIFactory1>);

    static CCryDX12GIFactory* Create();

    virtual ~CCryDX12GIFactory();

    IDXGIFactory1* GetDXGIFactory() const { return m_pDXGIFactory1; }

    #pragma region /* IDXGIFactory implementation */

    virtual HRESULT STDMETHODCALLTYPE EnumAdapters(
        UINT Adapter,
        _Out_ IDXGIAdapter** ppAdapter) final;

    virtual HRESULT STDMETHODCALLTYPE MakeWindowAssociation(
        HWND WindowHandle,
        UINT Flags) final;

    virtual HRESULT STDMETHODCALLTYPE GetWindowAssociation(
        _Out_ HWND* pWindowHandle) final;

    virtual HRESULT STDMETHODCALLTYPE CreateSwapChain(
        _In_ IUnknown* pDevice,
        _In_ DXGI_SWAP_CHAIN_DESC* pDesc,
        _Out_ IDXGISwapChain** ppSwapChain) final;

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION CCRYDX12GIFACTORY_HPP_SECTION_1
    #include AZ_RESTRICTED_FILE(CCryDX12GIFactory_hpp)
#endif

    virtual HRESULT STDMETHODCALLTYPE CreateSoftwareAdapter(
        HMODULE Module,
        _Out_ IDXGIAdapter** ppAdapter) final;

    #pragma endregion

    #pragma region /* IDXGIFactory1 implementation */

    virtual HRESULT STDMETHODCALLTYPE EnumAdapters1(
        UINT Adapter,
        _Out_ IDXGIAdapter1** ppAdapter) final;

    virtual BOOL STDMETHODCALLTYPE IsCurrent() final;

    #pragma endregion

protected:
    CCryDX12GIFactory(IDXGIFactory1* pDXGIFactory1);

private:
    DX12::SmartPtr<IDXGIFactory1> m_pDXGIFactory1;
};

#endif // __CRYDX12GIFACTORY__
