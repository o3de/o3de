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
#ifndef __DX12SWAPCHAIN__
#define __DX12SWAPCHAIN__

#include "DX12Base.hpp"
#include "DX12CommandList.hpp"
#include "DX12AsyncCommandQueue.hpp"

#include <AzCore/std/containers/vector.h>

class CCryDX12SwapChain;

namespace DX12
{
    class SwapChain : public AzRHI::ReferenceCounted
    {
    public:
        static SwapChain* Create(CommandList* pDevice, IDXGIFactory1* factory, DXGI_SWAP_CHAIN_DESC* pDesc);

    protected:
        SwapChain(CommandList* pDevice, IDXGISwapChain3* dxgiSwapChain, DXGI_SWAP_CHAIN_DESC* pDesc);

        virtual ~SwapChain();

    public:
        inline IDXGISwapChain3* GetDXGISwapChain() const
        {
            return m_NativeSwapChain;
        }

        inline Resource& GetBackBuffer(AZ::u32 index)
        {
            Resource* resource = m_BackBuffers[index];
            AZ_Assert(resource, "null backbuffer");
            return *resource;
        }
        inline Resource& GetCurrentBackBuffer()
        {
            Resource* resource = m_BackBuffers[m_NativeSwapChain->GetCurrentBackBufferIndex()];
            AZ_Assert(resource, "Resource is null");
            return *resource;
        }

        inline ResourceView& GetBackBufferView(AZ::u32 index)
        {
            return m_BackBufferViews[index];
        }

        inline ResourceView& GetCurrentBackBufferView()
        {
            return m_BackBufferViews[m_NativeSwapChain->GetCurrentBackBufferIndex()];
        }

        inline AZ::u32 GetCurrentBackBufferIndex() const
        {
            return m_NativeSwapChain->GetCurrentBackBufferIndex();
        }

        inline const DXGI_SWAP_CHAIN_DESC& GetDesc() const
        {
            return m_Desc;
        }

        HRESULT Present(AZ::u32 SyncInterval, AZ::u32 Flags);

    private:
        friend class ::CCryDX12SwapChain;
        void AcquireBuffers(AZStd::vector<Resource*>&& resource);
        void ForfeitBuffers();

        AsyncCommandQueue& m_AsyncQueue;
        CommandList* m_CommandList;

        DXGI_SWAP_CHAIN_DESC m_Desc;

        SmartPtr<IDXGISwapChain3> m_NativeSwapChain;

        AZStd::vector<Resource*> m_BackBuffers;
        AZStd::vector<ResourceView> m_BackBufferViews;
    };
}

#endif // __DX12SWAPCHAIN__
