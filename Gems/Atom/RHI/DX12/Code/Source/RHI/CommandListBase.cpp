/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/CommandListBase.h>
#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <RHI/NsightAftermath.h>
#include <AzCore/std/string/conversions.h>

namespace AZ
{
    namespace DX12
    {
        CommandListBase::~CommandListBase() {}

        void CommandListBase::Init(Device& device, RHI::HardwareQueueClass hardwareQueueClass, ID3D12CommandAllocator* commandAllocator)
        {
            Base::Init(device);
            m_hardwareQueueClass = hardwareQueueClass;

            AssertSuccess(device.GetDevice()->CreateCommandList(1, ConvertHardwareQueueClass(hardwareQueueClass), commandAllocator, nullptr, IID_GRAPHICS_PPV_ARGS(m_commandList.ReleaseAndGetAddressOf())));
            m_isRecording = true;

            if (device.IsAftermathInitialized())
            {
                m_aftermathCommandListContext = Aftermath::CreateAftermathContextHandle(GetCommandList(), device.GetAftermathGPUCrashTracker());
            }
        }

        void CommandListBase::Reset(ID3D12CommandAllocator* commandAllocator)
        {
            AZ_PROFILE_SCOPE(RHI, "CommandListBase: Reset");
            AZ_Assert(m_queuedBarriers.empty(), "Unflushed barriers in command list.");

            m_commandList->Reset(commandAllocator, nullptr);
            m_isRecording = true;
        }

        void CommandListBase::Close()
        {
            AZ_Assert(m_isRecording, "Attempting to close command list that isn't in a recording state");
            m_isRecording = false;

            AssertSuccess(m_commandList->Close());
        }

        void CommandListBase::SetNameInternal(const AZStd::string_view& name)
        {
            AZStd::fixed_wstring<256> wname;
            AZStd::to_wstring(wname, name.data());
            GetCommandList()->SetName(wname.data());
        }

        void CommandListBase::QueueUAVBarrier(ID3D12Resource* resource)
        {
            m_queuedBarriers.emplace_back();

            D3D12_RESOURCE_BARRIER& barrierDesc = m_queuedBarriers.back();
            barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
            barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrierDesc.UAV.pResource = resource;
        }

        bool CommandListBase::IsRecording() const
        {
            return m_isRecording;
        }

        ID3D12GraphicsCommandList* CommandListBase::GetCommandList()
        {
            return m_commandList.Get();
        }

        const ID3D12GraphicsCommandList* CommandListBase::GetCommandList() const
        {
            return m_commandList.Get();
        }

        RHI::HardwareQueueClass CommandListBase::GetHardwareQueueClass() const
        {
            return m_hardwareQueueClass;
        }

        void CommandListBase::SetAftermathEventMarker(const AZStd::string& markerData)
        {
            auto& device = static_cast<Device&>(GetDevice());
            Aftermath::SetAftermathEventMarker(m_aftermathCommandListContext, markerData, device.IsAftermathInitialized());
        }

        void CommandListBase::FlushBarriers()
        {
            if (m_queuedBarriers.size())
            {
                AZ_PROFILE_SCOPE(RHI, "CommandListBase: FlushBarriers");

                m_commandList->ResourceBarrier((UINT)m_queuedBarriers.size(), m_queuedBarriers.data());
                m_queuedBarriers.clear();
            }
        }

        void CommandListBase::QueueAliasingBarrier(const D3D12_RESOURCE_ALIASING_BARRIER& barrier)
        {
            m_queuedBarriers.emplace_back();

            D3D12_RESOURCE_BARRIER& barrierDesc = m_queuedBarriers.back();
            barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
            barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrierDesc.Aliasing = barrier;
        }

        void CommandListBase::QueueTransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter)
        {
            D3D12_RESOURCE_TRANSITION_BARRIER barrier;
            barrier.pResource = resource;
            barrier.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barrier.StateBefore = stateBefore;
            barrier.StateAfter = stateAfter;
            QueueTransitionBarrier(barrier);
        }

        void CommandListBase::QueueTransitionBarrier(const D3D12_RESOURCE_TRANSITION_BARRIER& transitionBarrier)
        {
            if (transitionBarrier.StateBefore != transitionBarrier.StateAfter)
            {
                m_queuedBarriers.emplace_back();

                D3D12_RESOURCE_BARRIER& barrierDesc = m_queuedBarriers.back();
                barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                barrierDesc.Transition = transitionBarrier;
            }
            else if (transitionBarrier.StateBefore == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
            {
                QueueUAVBarrier(transitionBarrier.pResource);
            }
        }
    }
}
