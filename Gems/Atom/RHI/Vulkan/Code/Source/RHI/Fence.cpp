/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/VkAllocator.h>
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <RHI/BinaryFence.h>
#include <RHI/Device.h>
#include <RHI/Fence.h>
#include <RHI/TimelineSemaphoreFence.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<Fence> Fence::Create()
        {
            return aznew Fence();
        }

        void Fence::SetSignalEvent(const AZStd::shared_ptr<AZ::Vulkan::SignalEvent>& signalEvent)
        {
            m_fenceImpl->SetSignalEvent(signalEvent);
        }

        void Fence::SetSignalEventBitToSignal(int bitToSignal)
        {
            m_fenceImpl->SetSignalEventBitToSignal(bitToSignal);
        }

        void Fence::SetSignalEventDependencies(SignalEvent::BitSet dependencies)
        {
            m_fenceImpl->SetSignalEventDependencies(dependencies);
        }

        void Fence::SignalEvent()
        {
            m_fenceImpl->SignalEvent();
        }

        FenceBase& Fence::GetFenceBase() const
        {
            return *m_fenceImpl;
        }

        void Fence::SetNameInternal(const AZStd::string_view& name)
        {
            m_fenceImpl->SetNameInternal(name);
        }

        RHI::ResultCode Fence::InitInternal(RHI::Device& baseDevice, RHI::FenceState initialState, bool usedForWaitingOnDevice)
        {
            if (usedForWaitingOnDevice)
            {
                AZ_Assert(baseDevice.GetFeatures().m_signalFenceFromCPU, "Fences cannot be waited for on the GPU.");
                m_fenceImpl = TimelineSemaphoreFence::Create();
            }
            else
            {
                m_fenceImpl = BinaryFence::Create(*this);
            }

            return m_fenceImpl->InitInternal(baseDevice, initialState);
        }

        void Fence::ShutdownInternal()
        {
            m_fenceImpl->Shutdown();
        }

        void Fence::SignalOnCpuInternal()
        {
            m_fenceImpl->SignalOnCpuInternal();
        }

        void Fence::WaitOnCpuInternal() const
        {
            m_fenceImpl->WaitOnCpuInternal();
        }

        void Fence::ResetInternal()
        {
            m_fenceImpl->ResetInternal();
        }

        RHI::FenceState Fence::GetFenceStateInternal() const
        {
            return m_fenceImpl->GetFenceStateInternal();
        }

        void Fence::SetExternallySignalled()
        {
            m_fenceImpl->SignalEvent();
        }
    } // namespace Vulkan
} // namespace AZ
