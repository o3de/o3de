/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/FrameCountMaxRingBuffer.h>
#include <Atom/RHI/DeviceRayTracingAccelerationStructure.h>
#include <Atom_RHI_Vulkan_Platform.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    namespace Vulkan
    {
        //! This class builds and contains the Vulkan RayTracing Cluster BLAS buffers.
        class RayTracingClusterBlas final : public RHI::DeviceRayTracingClusterBlas
        {
        public:
            AZ_CLASS_ALLOCATOR(RayTracingClusterBlas, AZ::SystemAllocator);

            static RHI::Ptr<RayTracingClusterBlas> Create();

            struct ClusterBlasBuffers
            {
                RHI::Ptr<RHI::DeviceBuffer> m_clasDstImplicitBuffer;
                RHI::Ptr<RHI::DeviceBuffer> m_clusterBlasDstImplicitBuffer;
                RHI::Ptr<RHI::DeviceBuffer> m_scratchDataBuffer;
                RHI::Ptr<RHI::DeviceBuffer> m_dstAddressesArrayBuffer;
                RHI::Ptr<RHI::DeviceBuffer> m_dstSizesArrayBuffer;
                RHI::Ptr<RHI::DeviceBuffer> m_blasSrcInfosBuffer;

                VkClusterAccelerationStructureTriangleClusterInputNV m_triangleClustersInput = {};
                VkClusterAccelerationStructureClustersBottomLevelInputNV m_clustersBottomLevelInput = {};

                VkClusterAccelerationStructureCommandsInfoNV m_buildClasCommandInfo = {};
                VkClusterAccelerationStructureCommandsInfoNV m_buildClusterBlasCommandInfo = {};
            };

            const ClusterBlasBuffers& GetBuffers() const
            {
                return m_buffers.GetCurrentElement();
            }

            uint64_t GetAccelerationStructureByteSize() override;

        private:
            RayTracingClusterBlas() = default;

            // RHI::DeviceRayTracingClusterBlas overrides
            RHI::ResultCode CreateBuffersInternal(
                RHI::Device& deviceBase,
                const RHI::DeviceRayTracingClusterBlasDescriptor* descriptor,
                const RHI::DeviceRayTracingBufferPools& rayTracingBufferPools) override;

            static VkBuildAccelerationStructureFlagsKHR GetAccelerationStructureBuildFlags(
                const RHI::RayTracingAccelerationStructureBuildFlags& buildFlags);

            // Buffer list to keep buffers alive for several frames
            RHI::FrameCountMaxRingBuffer<ClusterBlasBuffers> m_buffers;
        };
    } // namespace Vulkan
} // namespace AZ
