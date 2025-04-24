/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/VkAllocator.h>
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <Atom/RHI.Reflect/Vulkan/VulkanBus.h>
#include <RHI/Device.h>
#include <RHI/TimelineSemaphoreFence.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<FenceBase> TimelineSemaphoreFence::Create()
        {
            return aznew TimelineSemaphoreFence();
        }

        VkSemaphore TimelineSemaphoreFence::GetNativeSemaphore() const
        {
            return m_nativeSemaphore;
        }

        uint64_t TimelineSemaphoreFence::GetPendingValue() const
        {
            return m_pendingValue;
        }

        void TimelineSemaphoreFence::SetNameInternal(const AZStd::string_view& name)
        {
            if (IsInitialized() && !name.empty())
            {
                Debug::SetNameToObject(
                    reinterpret_cast<uint64_t>(m_nativeSemaphore), name.data(), VK_OBJECT_TYPE_FENCE, static_cast<Device&>(GetDevice()));
            }
        }

        RHI::ResultCode TimelineSemaphoreFence::InitInternal(RHI::Device& baseDevice, RHI::FenceState initialState)
        {
            RETURN_RESULT_IF_UNSUCCESSFUL(Base::InitInternal(baseDevice, initialState));

            auto& device = static_cast<Device&>(baseDevice);

            VkSemaphoreTypeCreateInfo timelineCreateInfo{};
            timelineCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
            timelineCreateInfo.pNext = nullptr;
            timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
            timelineCreateInfo.initialValue = 0;

            VkExternalSemaphoreHandleTypeFlags externalHandleTypeFlags = 0;
            ExternalHandleRequirementBus::Broadcast(
                &ExternalHandleRequirementBus::Events::CollectSemaphoreExportHandleTypes, externalHandleTypeFlags);
            VkExportSemaphoreCreateInfoKHR createExport{};
            if (externalHandleTypeFlags != 0)
            {
                createExport.sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO;
                createExport.handleTypes = externalHandleTypeFlags;
                timelineCreateInfo.pNext = &createExport;
            }

            VkSemaphoreCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            createInfo.pNext = &timelineCreateInfo;
            createInfo.flags = 0;

            const VkResult result =
                device.GetContext().CreateSemaphore(device.GetNativeDevice(), &createInfo, VkSystemAllocator::Get(), &m_nativeSemaphore);
            AssertSuccess(result);

            RETURN_RESULT_IF_UNSUCCESSFUL(ConvertResult(result));

            m_pendingValue = (initialState == RHI::FenceState::Signaled) ? 0 : 1;
            return RHI::ResultCode::Success;
        }

        void TimelineSemaphoreFence::ShutdownInternal()
        {
            if (m_nativeSemaphore != VK_NULL_HANDLE)
            {
                auto& device = static_cast<Device&>(GetDevice());
                device.GetContext().DestroySemaphore(device.GetNativeDevice(), m_nativeSemaphore, VkSystemAllocator::Get());
                m_nativeSemaphore = VK_NULL_HANDLE;
            }
        }

        void TimelineSemaphoreFence::SignalOnCpuInternal()
        {
            VkSemaphoreSignalInfo signalInfo;
            signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
            signalInfo.pNext = nullptr;
            signalInfo.semaphore = m_nativeSemaphore;
            signalInfo.value = m_pendingValue;

            auto& device = static_cast<Device&>(GetDevice());
            device.GetContext().SignalSemaphore(device.GetNativeDevice(), &signalInfo);
            SignalEvent();
        }

        void TimelineSemaphoreFence::WaitOnCpuInternal() const
        {
            VkSemaphoreWaitInfo waitInfo{};
            waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
            waitInfo.pNext = nullptr;
            waitInfo.flags = 0;
            waitInfo.semaphoreCount = 1;
            waitInfo.pSemaphores = &m_nativeSemaphore;

            // If another thread resets this Semaphore while we are waiting, m_pendingValue is changed, which might interfere
            // with the WaitSemaphore here, depending on how this is implemented in the driver.
            // To avoid this, make a local copy of the pending value
            auto pendingValue = m_pendingValue;
            waitInfo.pValues = &pendingValue;

            auto& device = static_cast<Device&>(GetDevice());
            device.GetContext().WaitSemaphores(device.GetNativeDevice(), &waitInfo, AZStd::numeric_limits<uint64_t>::max());
        }

        void TimelineSemaphoreFence::ResetInternal()
        {
            m_pendingValue++;
            m_inSignalledState = false;
        }

        RHI::FenceState TimelineSemaphoreFence::GetFenceStateInternal() const
        {
            auto& device = static_cast<Device&>(GetDevice());
            uint64_t completedValue = 0;
            device.GetContext().GetSemaphoreCounterValue(device.GetNativeDevice(), m_nativeSemaphore, &completedValue);
            return (m_pendingValue <= completedValue) ? RHI::FenceState::Signaled : RHI::FenceState::Reset;
        }
    } // namespace Vulkan
} // namespace AZ
