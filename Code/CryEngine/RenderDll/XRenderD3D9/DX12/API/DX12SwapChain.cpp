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
#include "DX12SwapChain.hpp"

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define DX12SWAPCHAIN_CPP_SECTION_1 1
#endif

namespace DX12
{
    SwapChain* SwapChain::Create(
        CommandList* commandList,
        IDXGIFactory1* pFactory,
        DXGI_SWAP_CHAIN_DESC* pDesc)
    {
        IDXGISwapChain* dxgiSwapChain = NULL;
        IDXGISwapChain3* dxgiSwapChain3 = NULL;
        ID3D12CommandQueue* commandQueue = commandList->GetD3D12CommandQueue();

        // If discard isn't implemented/supported/fails, try the newer swap-types
        // - flip_discard is win 10
        // - flip_sequentially is win 8
        HRESULT hr;
        if (pDesc->SwapEffect == DXGI_SWAP_EFFECT_DISCARD)
        {
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DX12SWAPCHAIN_CPP_SECTION_1
    #include AZ_RESTRICTED_FILE(DX12SwapChain_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else 
            pDesc->SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
#endif
            pDesc->BufferCount = std::max(2U, pDesc->BufferCount);
            hr = pFactory->CreateSwapChain(commandQueue, pDesc, &dxgiSwapChain);
        }
        else if (pDesc->SwapEffect == DXGI_SWAP_EFFECT_SEQUENTIAL)
        {
            pDesc->SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
            pDesc->BufferCount = std::max(2U, pDesc->BufferCount);
            hr = pFactory->CreateSwapChain(commandQueue, pDesc, &dxgiSwapChain);
        }
        else
        {
            hr = pFactory->CreateSwapChain(commandQueue, pDesc, &dxgiSwapChain);
        }

        if (hr == S_OK && dxgiSwapChain)
        {
            hr = dxgiSwapChain->QueryInterface(IID_PPV_ARGS(&dxgiSwapChain3));
            dxgiSwapChain->Release();

            if (hr == S_OK && dxgiSwapChain3)
            {
                return DX12::PassAddRef(new SwapChain(commandList, dxgiSwapChain3, pDesc));
            }
        }

        return nullptr;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    SwapChain::SwapChain(CommandList* commandList, IDXGISwapChain3* dxgiSwapChain, DXGI_SWAP_CHAIN_DESC* pDesc)
        : AzRHI::ReferenceCounted()
        , m_CommandList(commandList)
        , m_AsyncQueue(commandList->GetCommandListPool().GetAsyncCommandQueue())
    {
        if (pDesc)
        {
            m_Desc = *pDesc;
        }
        else
        {
            dxgiSwapChain->GetDesc(&m_Desc);
        }

        m_NativeSwapChain = dxgiSwapChain;
        dxgiSwapChain->Release();
    }

    SwapChain::~SwapChain()
    {
        ForfeitBuffers();

        m_NativeSwapChain->Release();
    }

    HRESULT SwapChain::Present(AZ::u32 SyncInterval, AZ::u32 Flags)
    {
        m_AsyncQueue.Flush();

        HRESULT hr = m_NativeSwapChain->Present(SyncInterval, Flags);
        return hr;
    }

    void SwapChain::AcquireBuffers(AZStd::vector<Resource*>&& buffers)
    {
        AZ_Assert(m_BackBuffers.empty(), "must forfeit buffers before assigning");

        m_BackBuffers = std::move(buffers);
        m_BackBufferViews.reserve(buffers.size());
        for (Resource* buffer : m_BackBuffers)
        {
            m_BackBufferViews.emplace_back();
            m_BackBufferViews.back().Init(*buffer, EVT_RenderTargetView);
        }
    }

    void SwapChain::ForfeitBuffers()
    {
        AZ_TRACE_METHOD();
        m_BackBuffers.clear();
        m_BackBufferViews.clear();
    }
}
