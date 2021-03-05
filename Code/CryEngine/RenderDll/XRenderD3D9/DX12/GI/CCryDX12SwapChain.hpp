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
#ifndef __CCRYDX12GISWAPCHAIN__
#define __CCRYDX12GISWAPCHAIN__

#include "DX12/CCryDX12Object.hpp"
#include "DX12/Resource/Texture/CCryDX12Texture2D.hpp"

#include "DX12/API/DX12SwapChain.hpp"

#include <AzCore/std/containers/vector.h>

class CCryDX12SwapChain
    : public CCryDX12GIObject<IDXGISwapChain3>
{
public:
    DX12_OBJECT(CCryDX12SwapChain, CCryDX12GIObject<IDXGISwapChain3>);

    static CCryDX12SwapChain* Create(CCryDX12Device* pDevice, IDXGIFactory1* factory, DXGI_SWAP_CHAIN_DESC* pDesc);

    virtual ~CCryDX12SwapChain();

    ILINE CCryDX12Device* GetDevice() const
    {
        return m_Device;
    }

    ILINE IDXGISwapChain3* GetDXGISwapChain() const
    {
        return m_SwapChain->GetDXGISwapChain();
    }

    ILINE DX12::SwapChain* GetDX12SwapChain()
    {
        return m_SwapChain;
    }

    #pragma region /* IDXGIDeviceSubObject implementation */

    virtual HRESULT STDMETHODCALLTYPE GetDevice(
        _In_  REFIID riid,
        _Out_  void** ppDevice) final;

    #pragma endregion

    #pragma region /* IDXGISwapChain implementation */

    virtual HRESULT STDMETHODCALLTYPE Present(
        UINT SyncInterval,
        UINT Flags) final;

    virtual HRESULT STDMETHODCALLTYPE GetBuffer(
        UINT Buffer,
        _In_  REFIID riid,
        _Out_  void** ppSurface) final;

    virtual HRESULT STDMETHODCALLTYPE SetFullscreenState(
        BOOL Fullscreen,
        _In_opt_  IDXGIOutput* pTarget) final;

    virtual HRESULT STDMETHODCALLTYPE GetFullscreenState(
        _Out_opt_  BOOL* pFullscreen,
        _Out_opt_  IDXGIOutput** ppTarget) final;

    virtual HRESULT STDMETHODCALLTYPE GetDesc(
        _Out_  DXGI_SWAP_CHAIN_DESC* pDesc) final;

    virtual HRESULT STDMETHODCALLTYPE ResizeBuffers(
        UINT BufferCount,
        UINT Width,
        UINT Height,
        DXGI_FORMAT NewFormat,
        UINT SwapChainFlags) final;

    virtual HRESULT STDMETHODCALLTYPE ResizeTarget(
        _In_  const DXGI_MODE_DESC* pNewTargetParameters) final;

    virtual HRESULT STDMETHODCALLTYPE GetContainingOutput(
        _Out_  IDXGIOutput** ppOutput) final;

    virtual HRESULT STDMETHODCALLTYPE GetFrameStatistics(
        _Out_  DXGI_FRAME_STATISTICS* pStats) final;

    virtual HRESULT STDMETHODCALLTYPE GetLastPresentCount(
        _Out_  UINT* pLastPresentCount) final;

    #pragma endregion

    #pragma region /* IDXGISwapChain1 implementation */

    virtual HRESULT STDMETHODCALLTYPE GetDesc1(
        /* [annotation][out] */
        [[maybe_unused]] _Out_  DXGI_SWAP_CHAIN_DESC1* pDesc) final
    {
        DX12_NOT_IMPLEMENTED
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE GetFullscreenDesc(
        /* [annotation][out] */
        [[maybe_unused]] _Out_  DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pDesc) final
    {
        DX12_NOT_IMPLEMENTED
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE GetHwnd(
        /* [annotation][out] */
        [[maybe_unused]] _Out_  HWND* pHwnd) final
    {
        DX12_NOT_IMPLEMENTED
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE GetCoreWindow(
        /* [annotation][in] */
        [[maybe_unused]] _In_  REFIID refiid,
        /* [annotation][out] */
        [[maybe_unused]] _COM_Outptr_  void** ppUnk) final
    {
        DX12_NOT_IMPLEMENTED
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE Present1(
        [[maybe_unused]] /* [in] */ UINT SyncInterval,
        [[maybe_unused]] /* [in] */ UINT PresentFlags,
        /* [annotation][in] */
        [[maybe_unused]] _In_  const DXGI_PRESENT_PARAMETERS* pPresentParameters) final
    {
        DX12_NOT_IMPLEMENTED
        return S_OK;
    }

    virtual BOOL STDMETHODCALLTYPE IsTemporaryMonoSupported() final
    {
        DX12_NOT_IMPLEMENTED
        return FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE GetRestrictToOutput(
        /* [annotation][out] */
        [[maybe_unused]] _Out_  IDXGIOutput** ppRestrictToOutput) final
    {
        DX12_NOT_IMPLEMENTED
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE SetBackgroundColor(
        /* [annotation][in] */
        [[maybe_unused]] _In_  const DXGI_RGBA* pColor) final
    {
        DX12_NOT_IMPLEMENTED
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE GetBackgroundColor(
        /* [annotation][out] */
        [[maybe_unused]] _Out_  DXGI_RGBA* pColor) final
    {
        DX12_NOT_IMPLEMENTED
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE SetRotation(
        /* [annotation][in] */
        [[maybe_unused]] _In_  DXGI_MODE_ROTATION Rotation) final
    {
        DX12_NOT_IMPLEMENTED
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE GetRotation(
        /* [annotation][out] */
        [[maybe_unused]] _Out_  DXGI_MODE_ROTATION* pRotation) final
    {
        DX12_NOT_IMPLEMENTED
        return S_OK;
    }

    #pragma endregion

    #pragma region /* IDXGISwapChain2 implementation */

    virtual HRESULT STDMETHODCALLTYPE SetSourceSize(
        [[maybe_unused]] UINT Width,
        [[maybe_unused]] UINT Height) final
    {
        DX12_NOT_IMPLEMENTED
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE GetSourceSize(
        /* [annotation][out] */
        [[maybe_unused]] _Out_  UINT* pWidth,
        /* [annotation][out] */
        [[maybe_unused]] _Out_  UINT* pHeight) final
    {
        DX12_NOT_IMPLEMENTED
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE SetMaximumFrameLatency(
        UINT MaxLatency) final
    {
        return m_SwapChain->GetDXGISwapChain()->SetMaximumFrameLatency(MaxLatency);
    }

    virtual HRESULT STDMETHODCALLTYPE GetMaximumFrameLatency(
        /* [annotation][out] */
        [[maybe_unused]] _Out_  UINT* pMaxLatency) final
    {
        DX12_NOT_IMPLEMENTED
        return S_OK;
    }

    virtual HANDLE STDMETHODCALLTYPE GetFrameLatencyWaitableObject() final
    {
        return m_SwapChain->GetDXGISwapChain()->GetFrameLatencyWaitableObject();
    }

    virtual HRESULT STDMETHODCALLTYPE SetMatrixTransform(
        [[maybe_unused]] const DXGI_MATRIX_3X2_F* pMatrix) final
    {
        DX12_NOT_IMPLEMENTED
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE GetMatrixTransform(
        /* [annotation][out] */
        [[maybe_unused]] _Out_  DXGI_MATRIX_3X2_F* pMatrix) final
    {
        DX12_NOT_IMPLEMENTED
        return S_OK;
    }

    #pragma endregion

    #pragma region /* IDXGISwapChain3 implementation */

    virtual UINT STDMETHODCALLTYPE GetCurrentBackBufferIndex() final;

    virtual HRESULT STDMETHODCALLTYPE CheckColorSpaceSupport(
        /* [annotation][in] */
        [[maybe_unused]] _In_  DXGI_COLOR_SPACE_TYPE ColorSpace,
        /* [annotation][out] */
        [[maybe_unused]] _Out_  UINT* pColorSpaceSupport) final
    {
        DX12_NOT_IMPLEMENTED
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE SetColorSpace1(
        /* [annotation][in] */
        [[maybe_unused]] _In_  DXGI_COLOR_SPACE_TYPE ColorSpace) final
    {
        DX12_NOT_IMPLEMENTED
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE ResizeBuffers1(
        /* [annotation][in] */
        [[maybe_unused]] _In_  UINT BufferCount,
        /* [annotation][in] */
        [[maybe_unused]] _In_  UINT Width,
        /* [annotation][in] */
        [[maybe_unused]] _In_  UINT Height,
        /* [annotation][in] */
        [[maybe_unused]] _In_  DXGI_FORMAT Format,
        /* [annotation][in] */
        [[maybe_unused]] _In_  UINT SwapChainFlags,
        /* [annotation][in] */
        [[maybe_unused]] _In_reads_(BufferCount) const UINT* pCreationNodeMask,
        /* [annotation][in] */
        [[maybe_unused]] _In_reads_(BufferCount) IUnknown* const* ppPresentQueue) final
    {
        DX12_NOT_IMPLEMENTED
        return S_OK;
    }

    #pragma endregion

protected:
    CCryDX12SwapChain(CCryDX12Device* pDevice, DX12::SwapChain* pSwapChain);

private:
    void AcquireBuffers();
    void ForfeitBuffers();

    CCryDX12Device* m_Device;

    DX12::SwapChain* m_SwapChain;
    AZStd::vector<DX12::SmartPtr<CCryDX12Texture2D>> m_BackBuffers;
};

#endif // __CCRYDX12GISWAPCHAIN__
