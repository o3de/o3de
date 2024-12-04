/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/Fence.h>
#include <RHI/Device.h>

#include <Atom/RHI.Reflect/DX12/DX12Bus.h>

namespace AZ
{
    namespace DX12
    {
        FenceEvent::FenceEvent(const char* name)
            : m_EventHandle(nullptr)
            , m_name(name)
        {
            m_EventHandle = CreateEvent(nullptr, false, false, nullptr);
        }

        FenceEvent::~FenceEvent()
        {
            CloseHandle(m_EventHandle);
        }

        const char* FenceEvent::GetName() const
        {
            return m_name;
        }

        RHI::ResultCode Fence::Init(ID3D12DeviceX* dx12Device, RHI::FenceState initialState)
        {
            Microsoft::WRL::ComPtr<ID3D12Fence> fencePtr;
            D3D12_FENCE_FLAGS flags = D3D12_FENCE_FLAG_NONE;
            DX12RequirementBus::Broadcast(&DX12RequirementBus::Events::CollectFenceFlags, flags);
            if (!AssertSuccess(dx12Device->CreateFence(0, flags, IID_GRAPHICS_PPV_ARGS(fencePtr.GetAddressOf()))))
            {
                AZ_Error("Fence", false, "Failed to initialize ID3D12Fence.");
                return RHI::ResultCode::Fail;
            }

            m_fence = fencePtr.Get();
            m_fence->Signal(0);
            m_pendingValue = (initialState == RHI::FenceState::Signaled) ? 0 : 1;
            return RHI::ResultCode::Success;
        }

        void Fence::Shutdown()
        {
            m_fence = nullptr;
        }

        void Fence::Wait(FenceEvent& fenceEvent) const
        {
            Wait(fenceEvent, m_pendingValue);
        }

        void Fence::Wait(FenceEvent& fenceEvent, uint64_t fenceValue) const
        {
            if (fenceValue > GetCompletedValue())
            {
                AZ_PROFILE_SCOPE(RHI, "Fence Wait: %s", fenceEvent.GetName());
                m_fence->SetEventOnCompletion(fenceValue, fenceEvent.m_EventHandle);
                WaitForSingleObject(fenceEvent.m_EventHandle, INFINITE);
            }
        }

        void Fence::Signal()
        {
            m_fence->Signal(m_pendingValue);
        }

        RHI::FenceState Fence::GetFenceState() const
        {
            const uint64_t completedValue = GetCompletedValue();
            return (m_pendingValue <= completedValue) ? RHI::FenceState::Signaled : RHI::FenceState::Reset;
        }

        uint64_t Fence::GetPendingValue() const
        {
            return m_pendingValue;
        }

        uint64_t Fence::GetCompletedValue() const
        {
            return m_fence->GetCompletedValue();
        }

        ID3D12Fence* Fence::Get() const
        {
            return m_fence.get();
        }

        uint64_t Fence::Increment()
        {
            return ++m_pendingValue;
        }

        void FenceSet::Init(ID3D12DeviceX* dx12Device, RHI::FenceState initialState)
        {
            for (uint32_t hardwareQueueIdx = 0; hardwareQueueIdx < RHI::HardwareQueueClassCount; ++hardwareQueueIdx)
            {
                m_fences[hardwareQueueIdx].Init(dx12Device, initialState);
            }
        }

        void FenceSet::Shutdown()
        {
            for (uint32_t hardwareQueueIdx = 0; hardwareQueueIdx < RHI::HardwareQueueClassCount; ++hardwareQueueIdx)
            {
                m_fences[hardwareQueueIdx].Shutdown();
            }
        }

        void FenceSet::Wait(FenceEvent& event) const
        {
            for (uint32_t hardwareQueueIdx = 0; hardwareQueueIdx < RHI::HardwareQueueClassCount; ++hardwareQueueIdx)
            {
                m_fences[hardwareQueueIdx].Wait(event);
            }
        }

        void FenceSet::Reset()
        {
            for (uint32_t hardwareQueueIdx = 0; hardwareQueueIdx < RHI::HardwareQueueClassCount; ++hardwareQueueIdx)
            {
                m_fences[hardwareQueueIdx].Increment();
            }
        }

        Fence& FenceSet::GetFence(RHI::HardwareQueueClass hardwareQueueClass)
        {
            return m_fences[static_cast<uint32_t>(hardwareQueueClass)];
        }

        const Fence& FenceSet::GetFence(RHI::HardwareQueueClass hardwareQueueClass) const
        {
            return m_fences[static_cast<uint32_t>(hardwareQueueClass)];
        }

        RHI::Ptr<FenceImpl> FenceImpl::Create()
        {
            return aznew FenceImpl();
        }

        RHI::ResultCode FenceImpl::InitInternal(RHI::Device& deviceBase, RHI::FenceState initialState, [[maybe_unused]] bool usedForWaitingOnDevice)
        {
            return m_fence.Init(static_cast<Device&>(deviceBase).GetDevice(), initialState);
        }

        void FenceImpl::ShutdownInternal()
        {
            m_fence.Shutdown();
        }

        void FenceImpl::ResetInternal()
        {
            m_fence.Increment();
        }

        void FenceImpl::SignalOnCpuInternal()
        {
            m_fence.Signal();
        }

        void FenceImpl::WaitOnCpuInternal() const
        {
            FenceEvent event("WaitOnCpu");
            m_fence.Wait(event);
        }

        RHI::FenceState FenceImpl::GetFenceStateInternal() const
        {
            return m_fence.GetFenceState();
        }
    }
}
