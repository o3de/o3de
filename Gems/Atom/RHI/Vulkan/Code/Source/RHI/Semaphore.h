/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceObject.h>
#include <Atom_RHI_Vulkan_Platform.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <RHI/SignalEvent.h>

namespace AZ
{
    namespace Vulkan
    {
        class Device;

        class Semaphore : public RHI::DeviceObject
        {
            using Base = RHI::DeviceObject;

        public:
            AZ_RTTI(Semaphore, "{A0946587-C4FD-49E7-BB6D-92EA80CE140E}", RHI::DeviceObject);
            AZ_CLASS_ALLOCATOR(Semaphore, AZ::ThreadPoolAllocator);

            using WaitSemaphore = AZStd::pair<VkPipelineStageFlags, RHI::Ptr<Semaphore>>;

            RHI::ResultCode Init(Device& device);
            ~Semaphore() = default;
            void Reset();

            void SetSignalEvent(const AZStd::shared_ptr<AZ::Vulkan::SignalEvent>& signalEvent);
            void SetSignalEventBitToSignal(int bitToSignal);
            void SetSignalEventDependencies(SignalEvent::BitSet dependencies);

            void SetRecycleValue(bool canRecycle);
            bool GetRecycleValue() const;
            VkSemaphore GetNativeSemaphore() const;

            virtual void SignalEvent();
            virtual void WaitEvent() const;

        protected:
            Semaphore() = default;
            virtual RHI::ResultCode InitInternal(Device& device) = 0;
            virtual void ResetInternal(){};

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void Shutdown() override;
            //////////////////////////////////////////////////////////////////////////

            AZStd::shared_ptr<AZ::Vulkan::SignalEvent> m_signalEvent;
            int m_bitToSignal = -1;
            SignalEvent::BitSet m_waitDependencies = 0;

            VkSemaphore m_nativeSemaphore = VK_NULL_HANDLE;
            bool m_recyclable = true;
        };
    }
}
