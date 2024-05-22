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
#include <RHI/Semaphore.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<Semaphore> BinarySemaphore::Create()
        {
            return aznew BinarySemaphore();
        }

        RHI::ResultCode BinarySemaphore::InitInternal(Device& device)
        {
            VkSemaphoreCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            createInfo.pNext = 0;
            createInfo.flags = 0;

            const VkResult result =
                device.GetContext().CreateSemaphore(device.GetNativeDevice(), &createInfo, VkSystemAllocator::Get(), &m_nativeSemaphore);
            AssertSuccess(result);

            RETURN_RESULT_IF_UNSUCCESSFUL(ConvertResult(result));

            SetName(GetName());
            return RHI::ResultCode::Success;
        }
    } // namespace Vulkan
} // namespace AZ
