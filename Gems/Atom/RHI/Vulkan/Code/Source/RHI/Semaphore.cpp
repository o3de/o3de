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
        RHI::ResultCode Semaphore::Init(Device& device)
        {
            Base::Init(device);
            return InitInternal(device);
        }

        void Semaphore::SetSignalEvent(const AZStd::shared_ptr<AZ::Vulkan::SignalEvent>& signalEvent)
        {
            m_signalEvent = signalEvent;
        }

        void Semaphore::SetSignalEventBitToSignal(int bitToSignal)
        {
            m_bitToSignal = bitToSignal;
        }

        void Semaphore::SetSignalEventDependencies(SignalEvent::BitSet dependencies)
        {
            m_waitDependencies = dependencies;
        }

        void Semaphore::SignalEvent()
        {
            if (m_signalEvent)
            {
                AZ_Assert(m_bitToSignal >= 0, "Fence: SignalEvent was not set");
                m_signalEvent->Signal(m_bitToSignal);
            }
        }

        void Semaphore::WaitEvent() const
        {
            if (m_signalEvent)
            {
                m_signalEvent->Wait(m_waitDependencies);
            }
        }

        void Semaphore::Reset()
        {
            m_signalEvent = {};
            ResetInternal();
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
            Base::Shutdown();
        }
    }
}
