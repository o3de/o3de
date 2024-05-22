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
#include <RHI/TimelineSemaphore.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<Semaphore> TimelineSemaphore::Create()
        {
            return aznew TimelineSemaphore();
        }

        RHI::ResultCode TimelineSemaphore::InitInternal(Device& device)
        {
            VkSemaphoreCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            createInfo.pNext = 0;
            createInfo.flags = 0;

            VkSemaphoreTypeCreateInfo timelineCreateInfo{};

            timelineCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
            timelineCreateInfo.pNext = nullptr;
            timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
            timelineCreateInfo.initialValue = 0;
            createInfo.pNext = &timelineCreateInfo;
            m_pendingValue = 1;

            const VkResult result =
                device.GetContext().CreateSemaphore(device.GetNativeDevice(), &createInfo, VkSystemAllocator::Get(), &m_nativeSemaphore);
            AssertSuccess(result);

            RETURN_RESULT_IF_UNSUCCESSFUL(ConvertResult(result));

            SetName(GetName());
            return RHI::ResultCode::Success;
        }

        uint64_t TimelineSemaphore::GetPendingValue()
        {
            return m_pendingValue;
        }

        void TimelineSemaphore::ResetInternal()
        {
            m_pendingValue++;
        }

        void TimelineSemaphore::WaitEvent() const
        {
            // We don't need for the signal event to be submitted for timeline semaphores
        }
    } // namespace Vulkan
} // namespace AZ
