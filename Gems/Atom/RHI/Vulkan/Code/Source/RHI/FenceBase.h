/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Fence.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <RHI/SignalEvent.h>

namespace AZ
{
    namespace Vulkan
    {
        class Device;

        class FenceBase : public RHI::Fence
        {
            using Base = RHI::Fence;

        public:
            AZ_CLASS_ALLOCATOR(FenceBase, AZ::ThreadPoolAllocator);
            AZ_RTTI(FenceBase, "{AAAD0A37-5F85-4A68-9464-06EDAC6D62B0}", Base);

            ~FenceBase() = default;

            void SetSignalEvent(const AZStd::shared_ptr<AZ::Vulkan::SignalEvent>& signalEvent);
            void SetSignalEventBitToSignal(int bitToSignal);
            void SetSignalEventDependencies(AZ::Vulkan::SignalEvent::BitSet dependencies);

            void SignalEvent();

        protected:
            FenceBase() = default;

            //////////////////////////////////////////////////////////////////////
            // RHI::Fence
            RHI::ResultCode InitInternal(RHI::Device& device, RHI::FenceState initialState) override;
            void ShutdownInternal() override;
            //////////////////////////////////////////////////////////////////////

            AZStd::shared_ptr<AZ::Vulkan::SignalEvent> m_signalEvent;
            int m_bitToSignal = -1;
            SignalEvent::BitSet m_waitDependencies = 0;
            bool m_inSignalledState = false;
        };
    } // namespace Vulkan
} // namespace AZ
