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
#ifndef __DX12RESOURCE__
#define __DX12RESOURCE__

#include "DX12Base.hpp"
#include "DX12/API/DX12CommandListFence.hpp"

#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace DX12
{
    class CommandList;
    class SwapChain;

    struct ResourceStates
    {
        ResourceStates()
            :m_CurrentState(static_cast<D3D12_RESOURCE_STATES>(-1)),
            m_AnnouncedState(static_cast<D3D12_RESOURCE_STATES>(-1))
        {}
        ResourceStates(D3D12_RESOURCE_STATES currentState, D3D12_RESOURCE_STATES announcedState)
            :m_CurrentState(currentState),
            m_AnnouncedState(announcedState)
        {}

        bool operator ==(const ResourceStates& states) const
        {
            return m_CurrentState == states.m_CurrentState && m_AnnouncedState == states.m_AnnouncedState;
        }
        bool operator !=(const ResourceStates& states) const
        {
            return !(*this == states);
        }
        D3D12_RESOURCE_STATES m_CurrentState;
        D3D12_RESOURCE_STATES m_AnnouncedState;
    };

    class Resource : public DeviceObject
    {
    public:
        Resource(Device* pDevice);
        virtual ~Resource();

        Resource(Resource&& r);
        Resource& operator=(Resource&& r);

        void TryStagingUpload(CommandList* pCmdList);
        bool Init(ID3D12Resource* pResource, D3D12_RESOURCE_STATES eInitialState, const D3D12_RESOURCE_DESC& desc);
        bool Init(ID3D12Resource* pResource, D3D12_RESOURCE_STATES eInitialState)
        {
            return Init(pResource, eInitialState, pResource->GetDesc());
        }

        // Initialization of contents
        struct InitialData
        {
            std::vector<D3D12_SUBRESOURCE_DATA> m_SubResourceData{};
            UINT64 m_Size{0};
            UINT64 m_UploadSize{0};
        };

        InitialData* GetOrCreateInitialData();

        inline bool InitHasBeenDeferred() const
        {
            return m_InitialData != nullptr;
        }

        // Swap-chain associativity
        inline void SetDX12SwapChain(SwapChain* pOwner)
        {
            m_pSwapChainOwner = pOwner;
        }

        inline SwapChain* GetDX12SwapChain() const
        {
            return m_pSwapChainOwner;
        }

        inline bool IsBackBuffer() const
        {
            return m_pSwapChainOwner != NULL;
        }

        inline bool IsOffCard() const
        {
            return m_HeapType == D3D12_HEAP_TYPE_READBACK || m_HeapType == D3D12_HEAP_TYPE_UPLOAD;
        }

        inline D3D12_RESOURCE_STATES GetRequiredResourceState() const
        {
            return m_HeapType == D3D12_HEAP_TYPE_READBACK ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_GENERIC_READ;
        }

        inline bool IsTarget() const
        {
            return !!(GetCurrentState() & (D3D12_RESOURCE_STATE_RENDER_TARGET | D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_DEPTH_WRITE));
        }

        inline bool IsGeneric() const
        {
            return !!(GetCurrentState() & (D3D12_RESOURCE_STATE_UNORDERED_ACCESS | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
        }

        inline D3D12_RESOURCE_DESC& GetDesc()
        {
            return m_Desc;
        }

        inline const D3D12_RESOURCE_DESC& GetDesc() const
        {
            return m_Desc;
        }

        UINT64 GetRequiredUploadSize(UINT FirstSubresource, UINT NumSubresources) const;

        inline ID3D12Resource* GetD3D12Resource() const
        {
            return m_pD3D12Resource;
        }
        inline D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const
        {
            return m_GPUVirtualAddress;
        }

        // Get current known resource barrier state
        inline D3D12_RESOURCE_STATES GetCurrentState() const
        {
            return m_States.m_CurrentState;
        }

        inline D3D12_RESOURCE_STATES GetAnnouncedState() const
        {
            return m_States.m_AnnouncedState;
        }

        inline void SetCurrentState(D3D12_RESOURCE_STATES state)
        {
            m_States.m_CurrentState = state;
        }

        inline void SetAnnouncedState(D3D12_RESOURCE_STATES state)
        {
            m_States.m_AnnouncedState = state;
        }

        UINT64 SetFenceValue(UINT64 fenceValue, const int id, const int type)
        {
            // Check submitted completed fence
            UINT64 utilizedValue = fenceValue;
            UINT64 previousValue = m_FenceValues[type][id];

#define DX12_FREETHREADED_RESOURCES
#ifndef DX12_FREETHREADED_RESOURCES
            m_FenceValues[type][id] = (previousValue > fenceValue ? previousValue : fenceValue);

            if (type == CMDTYPE_ANY)
            {
                m_FenceValues[CMDTYPE_READ ][id] =
                    m_FenceValues[CMDTYPE_WRITE][id] =
                        m_FenceValues[CMDTYPE_ANY][id];
            }
            else
            {
                m_FenceValues[CMDTYPE_ANY  ][id] = std::max(
                        m_FenceValues[CMDTYPE_READ ][id],
                        m_FenceValues[CMDTYPE_WRITE][id]);
            }
#else
            // CLs may submit in any order. Is it higher than last known completed fence? If so, update it!
            MaxFenceValue(m_FenceValues[type][id], fenceValue);

            if (type == CMDTYPE_ANY)
            {
                MaxFenceValue(m_FenceValues[CMDTYPE_READ ][id], m_FenceValues[CMDTYPE_ANY][id]);
                MaxFenceValue(m_FenceValues[CMDTYPE_WRITE][id], m_FenceValues[CMDTYPE_ANY][id]);
            }
            else
            {
                MaxFenceValue(m_FenceValues[CMDTYPE_ANY  ][id], std::max(m_FenceValues[CMDTYPE_READ][id], m_FenceValues[CMDTYPE_WRITE][id]));
            }
#endif // !NDEBUG

            return previousValue;
        }

        inline UINT64 GetFenceValue(const int id, const int type)
        {
            return m_FenceValues[type][id];
        }

        inline const std::atomic<AZ::u64> (& GetFenceValues(const int type))[CMDQUEUE_NUM]
        {
            return m_FenceValues[type];
        }

        inline void MaxFenceValues(std::atomic<AZ::u64> (&rFenceValues)[CMDTYPE_NUM][CMDQUEUE_NUM], const int type)
        {
            if ((type == CMDTYPE_READ) || (type == CMDTYPE_ANY))
            {
                MaxFenceValue(rFenceValues[CMDTYPE_READ ][CMDQUEUE_COPY    ], m_FenceValues[CMDTYPE_READ ][CMDQUEUE_COPY    ]);
                MaxFenceValue(rFenceValues[CMDTYPE_READ ][CMDQUEUE_GRAPHICS], m_FenceValues[CMDTYPE_READ ][CMDQUEUE_GRAPHICS]);
            }

            if ((type == CMDTYPE_WRITE) || (type == CMDTYPE_ANY))
            {
                MaxFenceValue(rFenceValues[CMDTYPE_WRITE][CMDQUEUE_COPY    ], m_FenceValues[CMDTYPE_WRITE][CMDQUEUE_COPY    ]);
                MaxFenceValue(rFenceValues[CMDTYPE_WRITE][CMDQUEUE_GRAPHICS], m_FenceValues[CMDTYPE_WRITE][CMDQUEUE_GRAPHICS]);
            }
        }

        bool IsUsed(CommandListPool& pCmdListPool, const int type = CMDTYPE_ANY);
        void WaitForUnused(CommandListPool& pCmdListPool, const int type = CMDTYPE_ANY);

        void MapDiscard(CommandList* pCmdList);

    protected:
        friend class CommandList;

        void DiscardInitialData();

        // Never changes after construction
        D3D12_RESOURCE_DESC m_Desc;
        D3D12_HEAP_TYPE m_HeapType;
        D3D12_GPU_VIRTUAL_ADDRESS m_GPUVirtualAddress;
        SmartPtr<ID3D12Resource> m_pD3D12Resource;  // can be replaced by MAP_DISCARD, which is deprecated

        SwapChain* m_pSwapChainOwner;

        // Potentially changes on every resource-use
        ResourceStates m_States;
        mutable std::atomic<AZ::u64> m_FenceValues[CMDTYPE_NUM][CMDQUEUE_NUM];

        // Used when using the resource the first time
        AZStd::unique_ptr<InitialData> m_InitialData;
    };
}

#endif // __DX12RESOURCE__
