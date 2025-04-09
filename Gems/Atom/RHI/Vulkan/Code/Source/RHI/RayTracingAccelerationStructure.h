/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceBuffer.h>
#include <Atom_RHI_Vulkan_Platform.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    namespace Vulkan
    {
        //! This class builds and contains the Vulkan RayTracing BLAS buffers.
        class RayTracingAccelerationStructure final : public RHI::DeviceObject
        {
        public:
            AZ_CLASS_ALLOCATOR(RayTracingAccelerationStructure, AZ::SystemAllocator);

            static RHI::Ptr<RayTracingAccelerationStructure> Create();

            void Init(Device& device, const VkAccelerationStructureCreateInfoKHR& createInfo);

            void Shutdown() override;

            bool IsValid() const
            {
                return m_accelerationStructure != VK_NULL_HANDLE;
            }

            VkAccelerationStructureKHR GetNativeAccelerationStructure()
            {
                return m_accelerationStructure;
            }

            void SetBlasBuffers(AZStd::vector<RHI::Ptr<RHI::DeviceBuffer>>&& buffers);

        private:
            RayTracingAccelerationStructure() = default;

            VkAccelerationStructureKHR m_accelerationStructure = VK_NULL_HANDLE;

            // need to keep a reference to the BLAS buffers to keep them alive as long as this AS is alive if this AS is a TLAS
            AZStd::vector<RHI::Ptr<RHI::DeviceBuffer>> m_blasBuffers;
        };
    } // namespace Vulkan
} // namespace AZ
