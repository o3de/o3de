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
        enum class SemaphoreType
        {
            Binary,
            Timeline,
            Invalid
        };

        class Semaphore final
            : public RHI::DeviceObject
        {
            using Base = RHI::DeviceObject;

        public:
            AZ_CLASS_ALLOCATOR(Semaphore, AZ::ThreadPoolAllocator);

            using WaitSemaphore = AZStd::pair<VkPipelineStageFlags, RHI::Ptr<Semaphore>>;

            static RHI::Ptr<Semaphore> Create();
            RHI::ResultCode Init(Device& device, bool forceBinarySemaphore);
            ~Semaphore() = default;

            SemaphoreType GetType();

            void SetSignalEvent(const AZStd::shared_ptr<AZ::Vulkan::SignalEvent>& signalEvent, int bitToSignal);
            void SetDependencies(const AZStd::shared_ptr<AZ::Vulkan::SignalEvent>& signalEvent, SignalEvent::BitSet dependencies);

            // Timeline semaphore functions
            uint64_t GetPendingValue();
            void IncrementPendingValue();

            // Binary semaphore functions
            void SignalEvent();
            void WaitEvent() const;
            void ResetSignalEvent();

            void SetRecycleValue(bool canRecycle);
            bool GetRecycleValue() const;
            VkSemaphore GetNativeSemaphore() const;

        private:
            Semaphore() = default;

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
            SemaphoreType m_type = SemaphoreType::Invalid;
            uint64_t m_pendingValue = 0;
        };
    }
}
