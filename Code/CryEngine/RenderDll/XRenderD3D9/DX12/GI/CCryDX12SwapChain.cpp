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
#include "RenderDll_precompiled.h"
#include "CCryDX12SwapChain.hpp"

#include "DX12/Device/CCryDX12Device.hpp"
#include "DX12/Device/CCryDX12DeviceContext.hpp"
#include "DX12/Resource/CCryDX12Resource.hpp"
#include "DX12/Resource/Texture/CCryDX12Texture1D.hpp"
#include "DX12/Resource/Texture/CCryDX12Texture2D.hpp"
#include "DX12/Resource/Texture/CCryDX12Texture3D.hpp"

#include "DX12/API/DX12Device.hpp"
#include "DX12/API/DX12SwapChain.hpp"
#include "DX12/API/DX12Resource.hpp"
#include "Dx12/API/DX12AsyncCommandQueue.hpp"

#include <AzCore/Debug/EventTraceDrillerBus.h>

#ifdef WIN32
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#endif

static const char* s_globalVsyncName = "V-Sync";
static const char* s_globalEmptyCategory = "";

static void RecordVSyncInstantEvent()
{
#ifdef WIN32
    DWM_TIMING_INFO timingInfo = { sizeof(DWM_TIMING_INFO) };
    HRESULT hr = DwmGetCompositionTimingInfo(NULL, &timingInfo);
    if (SUCCEEDED(hr))
    {
        UINT64 frequency;
        QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);
        UINT64 vsyncMicroSeconds = (timingInfo.qpcVBlank * 1000) / (frequency / 1000);
        EBUS_QUEUE_EVENT(AZ::Debug::EventTraceDrillerBus, RecordInstantGlobal, s_globalVsyncName, s_globalEmptyCategory, (AZ::u64)vsyncMicroSeconds);
    }
#endif
}


CCryDX12SwapChain* CCryDX12SwapChain::Create(CCryDX12Device* device, IDXGIFactory1* factory, DXGI_SWAP_CHAIN_DESC* pDesc)
{
    CCryDX12DeviceContext* deviceContext = device->GetDeviceContext();
    DX12::SwapChain* swapChain = DX12::SwapChain::Create(deviceContext->GetCoreGraphicsCommandList(), factory, pDesc);

    return DX12::PassAddRef(new CCryDX12SwapChain(device, swapChain));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CCryDX12SwapChain::CCryDX12SwapChain(CCryDX12Device* device, DX12::SwapChain* swapChain)
    : Super()
    , m_Device(device)
    , m_SwapChain(swapChain)
{
    DX12_FUNC_LOG
    AcquireBuffers();
}

CCryDX12SwapChain::~CCryDX12SwapChain()
{
    DX12_FUNC_LOG
    ForfeitBuffers();

    SAFE_RELEASE(m_SwapChain);
}

void CCryDX12SwapChain::AcquireBuffers()
{
    const DXGI_SWAP_CHAIN_DESC& desc = m_SwapChain->GetDesc();
    IDXGISwapChain* swapChain = m_SwapChain->GetDXGISwapChain();

    AZStd::vector<DX12::Resource*> resources;
    m_BackBuffers.reserve(desc.BufferCount);
    resources.reserve(desc.BufferCount);

    for (int i = 0; i < desc.BufferCount; ++i)
    {
        ID3D12Resource* resource12 = NULL;
        if (S_OK == swapChain->GetBuffer(i, IID_GRAPHICS_PPV_ARGS(&resource12)))
        {
            m_BackBuffers.emplace_back();
            m_BackBuffers.back().attach(CCryDX12Texture2D::Create(GetDevice(), this, resource12));
            resources.emplace_back(&m_BackBuffers.back()->GetDX12Resource());
            resource12->Release();
        }
    }

    m_SwapChain->AcquireBuffers(std::move(resources));
}

void CCryDX12SwapChain::ForfeitBuffers()
{
    m_SwapChain->ForfeitBuffers();
    m_BackBuffers.clear();
}

/* IDXGIDeviceSubObject implementation */

HRESULT STDMETHODCALLTYPE CCryDX12SwapChain::GetDevice(
    _In_  REFIID riid,
    _Out_  void** ppDevice)
{
    DX12_FUNC_LOG
    return m_SwapChain->GetDXGISwapChain()->GetDevice(riid, ppDevice);
}

/* IDXGISwapChain implementation */

HRESULT STDMETHODCALLTYPE CCryDX12SwapChain::Present(
    UINT SyncInterval,
    UINT Flags)
{
    AZ_TRACE_METHOD();
    DX12_FUNC_LOG

    m_Device->GetDeviceContext()->Finish(m_SwapChain);

    DX12_LOG("------------------------------------------------ PRESENT ------------------------------------------------");
    HRESULT hr = m_SwapChain->Present(SyncInterval, Flags);
    RecordVSyncInstantEvent();
    return hr;
}

HRESULT STDMETHODCALLTYPE CCryDX12SwapChain::GetBuffer(
    UINT Buffer,
    _In_  REFIID riid,
    _Out_  void** ppSurface)
{
    DX12_FUNC_LOG
    if (riid == __uuidof(ID3D11Texture2D))
    {
        AZ_Assert(Buffer < m_BackBuffers.size(), "Invalid buffer index");
        CCryDX12Texture2D* texture = m_BackBuffers[Buffer];
        texture->AddRef();
        *ppSurface = texture;
    }
    else
    {
        DX12_ASSERT(0, "Not implemented!");
        return -1;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CCryDX12SwapChain::SetFullscreenState(
    BOOL Fullscreen,
    _In_opt_  IDXGIOutput* pTarget)
{
    DX12_FUNC_LOG
    m_Device->GetDeviceContext()->SubmitAllCommands(true);
    return m_SwapChain->GetDXGISwapChain()->SetFullscreenState(Fullscreen, pTarget);
}

HRESULT STDMETHODCALLTYPE CCryDX12SwapChain::GetFullscreenState(
    _Out_opt_  BOOL* pFullscreen,
    _Out_opt_  IDXGIOutput** ppTarget)
{
    DX12_FUNC_LOG
    return m_SwapChain->GetDXGISwapChain()->GetFullscreenState(pFullscreen, ppTarget);
}

HRESULT STDMETHODCALLTYPE CCryDX12SwapChain::GetDesc(
    _Out_  DXGI_SWAP_CHAIN_DESC* pDesc)
{
    DX12_FUNC_LOG
    return m_SwapChain->GetDXGISwapChain()->GetDesc(pDesc);
}

HRESULT STDMETHODCALLTYPE CCryDX12SwapChain::ResizeBuffers(
    UINT BufferCount,
    UINT Width,
    UINT Height,
    DXGI_FORMAT NewFormat,
    UINT SwapChainFlags)
{
    DX12_FUNC_LOG

    ID3D11RenderTargetView* views[8] = {};
    m_Device->GetDeviceContext()->OMSetRenderTargets(8, views, nullptr);

    m_Device->GetDeviceContext()->SubmitAllCommands(true);

    m_Device->GetDeviceContext()->WaitForIdle();

    ForfeitBuffers();

    m_Device->GetDX12Device()->FlushReleaseHeap(DX12::Device::ResourceReleasePolicy::Immediate);

    HRESULT res = m_SwapChain->GetDXGISwapChain()->ResizeBuffers(BufferCount, Width, Height, NewFormat, SwapChainFlags);
    if (res == S_OK)
    {
        AcquireBuffers();
    }

    return res;
}

HRESULT STDMETHODCALLTYPE CCryDX12SwapChain::ResizeTarget(
    _In_  const DXGI_MODE_DESC* pNewTargetParameters)
{
    DX12_FUNC_LOG

    ID3D11RenderTargetView* views[8] = {};
    m_Device->GetDeviceContext()->OMSetRenderTargets(8, views, nullptr);

    m_Device->GetDeviceContext()->SubmitAllCommands(true);

    m_Device->GetDeviceContext()->WaitForIdle();

    ForfeitBuffers();

    HRESULT hr = m_SwapChain->GetDXGISwapChain()->ResizeTarget(pNewTargetParameters);
    return hr;
}

HRESULT STDMETHODCALLTYPE CCryDX12SwapChain::GetContainingOutput(
    _Out_  IDXGIOutput** ppOutput)
{
    DX12_FUNC_LOG
    return m_SwapChain->GetDXGISwapChain()->GetContainingOutput(ppOutput);
}

HRESULT STDMETHODCALLTYPE CCryDX12SwapChain::GetFrameStatistics(
    _Out_  DXGI_FRAME_STATISTICS* pStats)
{
    DX12_FUNC_LOG
    return m_SwapChain->GetDXGISwapChain()->GetFrameStatistics(pStats);
}

HRESULT STDMETHODCALLTYPE CCryDX12SwapChain::GetLastPresentCount(
    _Out_  UINT* pLastPresentCount)
{
    DX12_FUNC_LOG
    return m_SwapChain->GetDXGISwapChain()->GetLastPresentCount(pLastPresentCount);
}

/* IDXGISwapChain1 implementation */

/* IDXGISwapChain2 implementation */

/* IDXGISwapChain3 implementation */

UINT STDMETHODCALLTYPE CCryDX12SwapChain::GetCurrentBackBufferIndex(
    void)
{
    DX12_FUNC_LOG
    return m_SwapChain->GetCurrentBackBufferIndex();
}
