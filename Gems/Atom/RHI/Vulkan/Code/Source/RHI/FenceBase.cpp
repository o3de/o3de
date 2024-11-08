/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/VkAllocator.h>
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <RHI/Device.h>
#include <RHI/FenceBase.h>

namespace AZ
{
    namespace Vulkan
    {
        void FenceBase::SetSignalEvent(const AZStd::shared_ptr<AZ::Vulkan::SignalEvent>& signalEvent)
        {
            m_signalEvent = signalEvent;
        }

        void FenceBase::SetSignalEventBitToSignal(int bitToSignal)
        {
            AZ_Assert(m_signalEvent, "Fence: Signal event must be set before setting bit to signal");
            m_bitToSignal = bitToSignal;
            if (m_inSignalledState)
            {
                SignalEvent();
            }
        }

        void FenceBase::SetSignalEventDependencies(AZ::Vulkan::SignalEvent::BitSet dependencies)
        {
            m_waitDependencies = dependencies;
        }

        void FenceBase::SignalEvent()
        {
            if (m_signalEvent)
            {
                AZ_Assert(m_bitToSignal >= 0, "Fence: SignaEvent was not set");
                m_signalEvent->Signal(m_bitToSignal);
            }
            m_inSignalledState = true;
        }

        RHI::ResultCode FenceBase::InitInternal(RHI::Device& baseDevice, RHI::FenceState initialState)
        {
            switch (initialState)
            {
            case RHI::FenceState::Reset:
                m_inSignalledState = false;
                break;
            case RHI::FenceState::Signaled:
                m_inSignalledState = true;
                break;
            default:
                return RHI::ResultCode::InvalidArgument;
            }
            DeviceObject::Init(baseDevice);
            return RHI::ResultCode::Success;
        }

        void FenceBase::Shutdown()
        {
            ShutdownInternal();
            Base::Shutdown();
        }
    } // namespace Vulkan
} // namespace AZ
