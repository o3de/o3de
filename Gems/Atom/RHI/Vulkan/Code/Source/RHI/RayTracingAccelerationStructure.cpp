/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Atom_RHI_Vulkan_Platform.h"
#include <Atom/RHI.Reflect/VkAllocator.h>
#include <RHI/Device.h>
#include <RHI/RayTracingAccelerationStructure.h>

namespace AZ::Vulkan
{
    RHI::Ptr<RayTracingAccelerationStructure> RayTracingAccelerationStructure::Create()
    {
        return new RayTracingAccelerationStructure;
    }

    void RayTracingAccelerationStructure::Init(Device& device, const VkAccelerationStructureCreateInfoKHR& createInfo)
    {
        VkResult vkResult = device.GetContext().CreateAccelerationStructureKHR(
            device.GetNativeDevice(), &createInfo, VkSystemAllocator::Get(), &m_accelerationStructure);
        AssertSuccess(vkResult);
        DeviceObject::Init(device);
    }

    void RayTracingAccelerationStructure::Shutdown()
    {
        m_blasBuffers.clear();
        auto& device = static_cast<Device&>(GetDevice());
        device.GetContext().DestroyAccelerationStructureKHR(device.GetNativeDevice(), m_accelerationStructure, VkSystemAllocator::Get());
        m_accelerationStructure = VK_NULL_HANDLE;
        RHI::DeviceObject::Shutdown();
    }

    void RayTracingAccelerationStructure::SetBlasBuffers(AZStd::vector<RHI::Ptr<RHI::DeviceBuffer>>&& buffers)
    {
        m_blasBuffers = buffers;
    }
} // namespace AZ::Vulkan
