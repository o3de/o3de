/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <RHI/Device.h>
#include <RHI/Semaphore.h>
#include <Atom/RHI.Reflect/VkAllocator.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<Semaphore> Semaphore::Create()
        {
            return aznew Semaphore();
        }

        RHI::ResultCode Semaphore::Init(Device& device, bool forceBinarySemaphore)
        {
            Base::Init(device);

            VkSemaphoreCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            createInfo.pNext = 0;
            createInfo.flags = 0;

            VkSemaphoreTypeCreateInfo timelineCreateInfo{};
            if (device.GetFeatures().m_signalFenceFromCPU && !forceBinarySemaphore)
            {
                m_type = SemaphoreType::Timeline;

                timelineCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
                timelineCreateInfo.pNext = nullptr;
                timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
                timelineCreateInfo.initialValue = 0;
                createInfo.pNext = &timelineCreateInfo;
                m_pendingValue = 1;
            }
            else
            {
                m_type = SemaphoreType::Binary;
            }

            const VkResult result =
                device.GetContext().CreateSemaphore(device.GetNativeDevice(), &createInfo, VkSystemAllocator::Get(), &m_nativeSemaphore);
            AssertSuccess(result);

            RETURN_RESULT_IF_UNSUCCESSFUL(ConvertResult(result));

            SetName(GetName());
            return RHI::ResultCode::Success;
        }

        SemaphoreType Semaphore::GetType()
        {
            return m_type;
        }

        void Semaphore::SetSignalEvent(const AZStd::shared_ptr<AZ::Vulkan::SignalEvent>& signalEvent, int bitToSignal)
        {
            m_signalEvent = signalEvent;
            m_bitToSignal = bitToSignal;
            // TODO signal if the fence is in a signalled state
        }

        void Semaphore::SetDependencies(const AZStd::shared_ptr<AZ::Vulkan::SignalEvent>& signalEvent, SignalEvent::BitSet dependencies)
        {
            m_signalEvent = signalEvent;
            m_waitDependencies = dependencies;
        }

        uint64_t Semaphore::GetPendingValue()
        {
            AZ_Assert(m_type == SemaphoreType::Timeline, "%s: Invalid Semaphore Type", __FUNCTION__);
            return m_pendingValue;
        }

        void Semaphore::IncrementPendingValue()
        {
            AZ_Assert(m_type == SemaphoreType::Timeline, "%s: Invalid Semaphore Type", __FUNCTION__);
            m_pendingValue++;
            m_signalEvent = {};
        }

        void Semaphore::SignalEvent()
        {
            if (m_signalEvent)
            {
                AZ_Assert(m_bitToSignal >= 0, "Fence: Signavent was not set");
                m_signalEvent->Signal(m_bitToSignal);
            }
        }

        void Semaphore::WaitEvent() const
        {
            if (m_type == SemaphoreType::Binary && m_signalEvent)
            {
                m_signalEvent->Wait(m_waitDependencies);
            }
        }

        void Semaphore::ResetSignalEvent()
        {
            AZ_Assert(m_type == SemaphoreType::Binary, "%s: Invalid Semaphore Type", __FUNCTION__);
            m_signalEvent = {};
        }

        void Semaphore::SetRecycleValue(bool canRecycle)
        {
            m_recyclable = canRecycle;
        }

        bool Semaphore::GetRecycleValue() const
        {
            return m_recyclable;
        }

        VkSemaphore Semaphore::GetNativeSemaphore() const
        {
            return m_nativeSemaphore;
        }

        void Semaphore::SetNameInternal(const AZStd::string_view& name)
        {
            if (IsInitialized() && !name.empty())
            {
                Debug::SetNameToObject(reinterpret_cast<uint64_t>(m_nativeSemaphore), name.data(), VK_OBJECT_TYPE_SEMAPHORE, static_cast<Device&>(GetDevice()));
            }
        }

        void Semaphore::Shutdown()
        {
            if (m_nativeSemaphore != VK_NULL_HANDLE)
            {
                auto& device = static_cast<Device&>(GetDevice());
                device.GetContext().DestroySemaphore(device.GetNativeDevice(), m_nativeSemaphore, VkSystemAllocator::Get());
                m_nativeSemaphore = VK_NULL_HANDLE;
            }
            // Signal any pending threads.
            SignalEvent();
            Base::Shutdown();
        }
    }
}
