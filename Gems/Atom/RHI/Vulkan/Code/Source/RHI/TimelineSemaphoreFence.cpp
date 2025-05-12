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
#include <Vulkan_Fence_Platform.h>

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
            return m_originalDeviceFence ? m_originalDeviceFence->m_pendingValue : m_pendingValue;
        }

        void TimelineSemaphoreFence::SetNameInternal(const AZStd::string_view& name)
        {
            if (IsInitialized() && !name.empty())
            {
                Debug::SetNameToObject(
                    reinterpret_cast<uint64_t>(m_nativeSemaphore), name.data(), VK_OBJECT_TYPE_FENCE, static_cast<Device&>(GetDevice()));
            }
        }

        RHI::ResultCode TimelineSemaphoreFence::InitInternal(RHI::Device& baseDevice, RHI::FenceState initialState, bool usedForCrossDevice)
        {
            RETURN_RESULT_IF_UNSUCCESSFUL(Base::InitInternal(baseDevice, initialState, usedForCrossDevice));

            auto& device = static_cast<Device&>(baseDevice);

            void* pNext = nullptr;
            VkSemaphoreTypeCreateInfo timelineCreateInfo{};
            timelineCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
            timelineCreateInfo.pNext = nullptr;
            timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
            timelineCreateInfo.initialValue = 0;
            pNext = &timelineCreateInfo;

            VkExternalSemaphoreHandleTypeFlags externalHandleTypeFlags = 0;
            ExternalHandleRequirementBus::Broadcast(
                &ExternalHandleRequirementBus::Events::CollectSemaphoreExportHandleTypes, externalHandleTypeFlags);
#if AZ_VULKAN_CROSS_DEVICE_SEMAPHORES_SUPPORTED
            if (usedForCrossDevice)
            {
                externalHandleTypeFlags |= ExternalSemaphoreHandleTypeBit;
            }
#endif
            VkExportSemaphoreCreateInfoKHR createExport{};
            if (externalHandleTypeFlags != 0)
            {
                createExport.sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO;
                createExport.handleTypes = externalHandleTypeFlags;
                createExport.pNext = pNext;
                pNext = &createExport;
            }

            VkSemaphoreCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            createInfo.pNext = pNext;
            createInfo.flags = 0;

            const VkResult result =
                device.GetContext().CreateSemaphore(device.GetNativeDevice(), &createInfo, VkSystemAllocator::Get(), &m_nativeSemaphore);
            AssertSuccess(result);

            RETURN_RESULT_IF_UNSUCCESSFUL(ConvertResult(result));

            m_pendingValue = (initialState == RHI::FenceState::Signaled) ? 0 : 1;
            return RHI::ResultCode::Success;
        }

        RHI::ResultCode TimelineSemaphoreFence::InitCrossDeviceInternal(RHI::Device& baseDevice, RHI::Ptr<Fence> originalDeviceFence)
        {
#if AZ_VULKAN_CROSS_DEVICE_SEMAPHORES_SUPPORTED
            auto originalTimelineSemaphoreFence = azrtti_cast<TimelineSemaphoreFence*>(&originalDeviceFence->GetFenceBase());
            if (originalTimelineSemaphoreFence == nullptr)
            {
                AZ_Assert(false, "Cannot create a cross device TimelineSemaphoreFence from a BinaryFence");
            }
            m_originalDeviceFence = originalTimelineSemaphoreFence;

            InitInternal(baseDevice, RHI::FenceState::Reset, true);
            auto& device = static_cast<Device&>(baseDevice);
            auto& originalDevice = static_cast<Device&>(originalDeviceFence->GetDevice());
            AZ_Assert(
                static_cast<const PhysicalDevice&>(device.GetPhysicalDevice())
                    .IsOptionalDeviceExtensionSupported(OptionalDeviceExtension::ExternalSemaphore),
                "External semaphores are not supported on device %d",
                device.GetDeviceIndex());
            AZ_Assert(
                static_cast<const PhysicalDevice&>(originalDevice.GetPhysicalDevice())
                    .IsOptionalDeviceExtensionSupported(OptionalDeviceExtension::ExternalSemaphore),
                "External semaphores are not supported on device %d",
                originalDevice.GetDeviceIndex());

            auto result =
                ImportCrossDeviceSemaphore(originalDevice, originalTimelineSemaphoreFence->GetNativeSemaphore(), device, m_nativeSemaphore);
            AZ_Assert(result == VK_SUCCESS, "Importing semaphore failed");
            return ConvertResult(result);
#else
            AZ_Assert(false, "Cross Device Fences are not supported on this platform");
            return RHI::ResultCode::Fail;
#endif
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
            signalInfo.value = GetPendingValue();

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
            auto pendingValue = GetPendingValue();
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
            if (m_originalDeviceFence)
            {
                return m_originalDeviceFence->GetFenceStateInternal();
            }
            else
            {
                auto& device = static_cast<Device&>(GetDevice());
                uint64_t completedValue = 0;
                device.GetContext().GetSemaphoreCounterValue(device.GetNativeDevice(), m_nativeSemaphore, &completedValue);
                return (m_pendingValue <= completedValue) ? RHI::FenceState::Signaled : RHI::FenceState::Reset;
            }
        }
    } // namespace Vulkan
} // namespace AZ
