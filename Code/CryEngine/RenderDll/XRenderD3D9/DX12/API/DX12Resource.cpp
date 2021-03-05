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
#include "DX12Resource.hpp"
#include "DX12Device.hpp"

namespace DX12
{
    Resource::Resource(Device* pDevice)
        : DeviceObject(pDevice)
        , m_HeapType(D3D12_HEAP_TYPE_DEFAULT)
        , m_GPUVirtualAddress(DX12_GPU_VIRTUAL_ADDRESS_NULL)
        , m_pD3D12Resource(nullptr)
        , m_States(D3D12_RESOURCE_STATE_COMMON, static_cast<D3D12_RESOURCE_STATES>(-1))
        , m_pSwapChainOwner(NULL)
    {
        memset(&m_FenceValues, 0, sizeof(m_FenceValues));
    }

    Resource::Resource(Resource&& r)
        : DeviceObject(std::move(r))
        , m_HeapType(std::move(r.m_HeapType))
        , m_GPUVirtualAddress(std::move(r.m_GPUVirtualAddress))
        , m_pD3D12Resource(std::move(r.m_pD3D12Resource))
        , m_States(std::move(r.m_States))
        , m_InitialData(std::move(r.m_InitialData))
        , m_pSwapChainOwner(std::move(r.m_pSwapChainOwner))
    {
        memcpy(&m_FenceValues, &r.m_FenceValues, sizeof(m_FenceValues));

        r.m_pD3D12Resource = nullptr;
        r.m_pSwapChainOwner = nullptr;
    }

    Resource& Resource::operator=(Resource&& r)
    {
        DeviceObject::operator=(std::move(r));

        m_HeapType = std::move(r.m_HeapType);
        m_GPUVirtualAddress = std::move(r.m_GPUVirtualAddress);
        m_pD3D12Resource = std::move(r.m_pD3D12Resource);
        m_States = std::move(r.m_States);
        m_InitialData = std::move(r.m_InitialData);
        m_pSwapChainOwner = std::move(r.m_pSwapChainOwner);

        memcpy(&m_FenceValues, &r.m_FenceValues, sizeof(m_FenceValues));

        r.m_pD3D12Resource = nullptr;
        r.m_pSwapChainOwner = nullptr;

        return *this;
    }

    Resource::~Resource()
    {
        m_pD3D12Resource->AddRef();

        GetDevice()->ReleaseLater(m_pD3D12Resource, GetCurrentState(), GetAnnouncedState());
    }

    UINT64 Resource::GetRequiredUploadSize(UINT FirstSubresource, UINT NumSubresources) const
    {
        UINT64 Size;
        GetDevice()->GetD3D12Device()->GetCopyableFootprints(&m_Desc, FirstSubresource, NumSubresources, 0, nullptr, nullptr, nullptr, &Size);
        return Size;
    }

    bool Resource::IsUsed(CommandListPool& commandListPool, const int type)
    {
        return !commandListPool.IsCompleted(m_FenceValues[type]);
    }

    void Resource::WaitForUnused(CommandListPool& commandListPool, const int type)
    {
        commandListPool.WaitForFenceOnCPU(m_FenceValues[type]);
    }

    bool Resource::Init(ID3D12Resource* pResource, D3D12_RESOURCE_STATES eInitialState, const D3D12_RESOURCE_DESC& desc)
    {
        m_pD3D12Resource = pResource;
        m_Desc = desc;

        D3D12_HEAP_PROPERTIES sHeap;

        if (m_pD3D12Resource->GetHeapProperties(&sHeap, nullptr) == S_OK)
        {
            m_HeapType = sHeap.Type;

            if (m_HeapType == D3D12_HEAP_TYPE_UPLOAD)
            {
                SetCurrentState(D3D12_RESOURCE_STATE_GENERIC_READ);
            }
            else if (m_HeapType == D3D12_HEAP_TYPE_READBACK)
            {
                SetCurrentState(D3D12_RESOURCE_STATE_COPY_DEST);
            }
            else
            {
                SetCurrentState(eInitialState);
            }
        }

        if (pResource && desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
        {
            m_GPUVirtualAddress = m_pD3D12Resource->GetGPUVirtualAddress();
        }

        m_pSwapChainOwner = NULL;

        return true;
    }

    void Resource::TryStagingUpload(CommandList* commandList)
    {
        if (m_InitialData)
        {
            DX12_LOG("Uploading initial data");
            DX12_ASSERT(!IsUsed(commandList->GetCommandListPool()), "Resource is used prior to initialization!");

            AZStd::unique_ptr<InitialData> initialData = AZStd::move(m_InitialData);

            commandList->UpdateSubresources(*this,
                initialData->m_UploadSize,
                static_cast<UINT>(initialData->m_SubResourceData.size()),
                &(initialData->m_SubResourceData[0]));
        }
    }

    Resource::InitialData* Resource::GetOrCreateInitialData()
    {
        if (!m_InitialData)
        {
            m_InitialData.reset(new InitialData());
        }

        return m_InitialData.get();
    }

    void Resource::MapDiscard(CommandList* pCmdList)
    {
        m_pD3D12Resource->AddRef();

        GetDevice()->ReleaseLater(m_pD3D12Resource, GetCurrentState(), GetAnnouncedState());

        D3D12_RESOURCE_STATES usage = D3D12_RESOURCE_STATE_GENERIC_READ;
        D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_UPLOAD;
        ID3D12Resource* resource = NULL;

        ResourceStates resourceStates;
        const CD3DX12_HEAP_PROPERTIES heapProperties(heapType);
        if (S_OK != GetDevice()->CreateOrReuseCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
                &m_Desc,
                usage,
                NULL,
                IID_GRAPHICS_PPV_ARGS(&resource),
                resourceStates
                ))
        {
            DX12_ASSERT(0, "Could not create buffer resource!");
            return;
        }

        m_pD3D12Resource = resource;
        
        resource->Release();

        if (m_pD3D12Resource && m_Desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
        {
            m_GPUVirtualAddress = m_pD3D12Resource->GetGPUVirtualAddress();
        }

        SetCurrentState(usage);
        SetAnnouncedState(static_cast<D3D12_RESOURCE_STATES>(-1));

        // If it is reused resource,  we have to transition the state to new state
        if(m_States != resourceStates)
            pCmdList->QueueTransitionBarrier(*this, resourceStates.m_CurrentState);
    }
}
