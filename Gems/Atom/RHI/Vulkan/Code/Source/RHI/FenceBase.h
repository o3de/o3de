/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceFence.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <RHI/SignalEvent.h>

namespace AZ
{
    namespace Vulkan
    {
        class Device;
        class Fence;

        class FenceBase : public RHI::DeviceObject
        {
            using Base = RHI::DeviceObject;
            friend class AZ::Vulkan::Fence;

        public:
            AZ_CLASS_ALLOCATOR(FenceBase, AZ::ThreadPoolAllocator);
            AZ_RTTI(FenceBase, "{AAAD0A37-5F85-4A68-9464-06EDAC6D62B0}", RHI::DeviceObject);

            void SetSignalEvent(const AZStd::shared_ptr<AZ::Vulkan::SignalEvent>& signalEvent);
            void SetSignalEventBitToSignal(int bitToSignal);
            void SetSignalEventDependencies(AZ::Vulkan::SignalEvent::BitSet dependencies);

            void SignalEvent();

        protected:
            FenceBase() = default;

            //////////////////////////////////////////////////////////////////////
            // RHI::Object
            virtual void SetNameInternal(const AZStd::string_view& name) = 0;
            //////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////
            // Interface
            virtual RHI::ResultCode InitInternal(RHI::Device& device, RHI::FenceState initialState);
            virtual void ShutdownInternal() = 0;
            virtual void Shutdown();
            virtual void SignalOnCpuInternal() = 0;
            virtual void WaitOnCpuInternal() const = 0;
            virtual void ResetInternal() = 0;
            virtual RHI::FenceState GetFenceStateInternal() const = 0;

            //////////////////////////////////////////////////////////////////////

            AZStd::shared_ptr<AZ::Vulkan::SignalEvent> m_signalEvent;
            int m_bitToSignal = -1;
            SignalEvent::BitSet m_waitDependencies = 0;
            bool m_inSignalledState = false;
        };
    } // namespace Vulkan
} // namespace AZ
