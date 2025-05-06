/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/FrameCountMaxRingBuffer.h>
#include <Atom_RHI_Vulkan_Platform.h>
#include <Atom/RHI/DeviceRayTracingAccelerationStructure.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    namespace Vulkan
    {
        class Buffer;
        class RayTracingAccelerationStructure;

        //! This class builds and contains the Vulkan RayTracing Cluster BLAS buffers.
        class RayTracingClusterBlas final
            : public RHI::DeviceRayTracingClusterBlas
        {
        public:
            AZ_CLASS_ALLOCATOR(RayTracingClusterBlas, AZ::SystemAllocator);

            static RHI::Ptr<RayTracingClusterBlas> Create();

            // The whole building process should be like this:
            // 1. Build CLASes of each cluster BLAS with N commands. The addresses of built CLASes is stored into m_dstAddressesArrayBuffer within each cluster BLAS.
            // 2. Build all cluster BLASes with 1 command or Build each cluster BLAS with N commands:
            //      1) fill m_srcInfosArrayBuffer with addresses of m_dstAddressesArrayBuffer of each cluster BLAS
            //      2) build all cluster BLASes in one command or build each cluster BLAS with N commands. The addresses of built cluster BLASes is sotred into m_dstAddressesArrayBuffer.
            // 3. Build TLAS:
            //      1) fill instance buffer with addresses of m_dstAddressesArrayBuffer of each cluster BLAS
            //      2) build each TLAS

            // The function calls should be like this:
            // 0. construct cluster BLAS/TLAS structs with descriptors
            // 1. a customized pass that fill m_srcInfosArrayBuffer and m_srcInfosCountBuffer, that provide vertex data for building CLASes
            // 2. a pass that invokes CommandList::BuildClusterAccelerationStructures, that build CLASes of each cluster BLAS
            // 3. a customized pass that fill m_srcInfosArrayBuffer.clusterReferences with device address of m_dstAddressesArrayBuffer and m_srcInfosCountBuffer, that provide CLASes compositing the cluster BLAS
            // 4. a pass that invokes CommandList::BuildClusterBottomLevelAccelerationStructure, that build each cluster BLAS
            // 5. a customized pass that fill instanceBuffer.accelerationStructureReference with device address of cluster BLASes, that provide cluster BLASes compositing the TLAS
            // 6. a pass(RayTracingAccelerationStructurePass) that invokes CommandList::BuildTopLevelAccelerationStructure, that build the TLAS


            struct ClusterBlasBuffers
            {
                // memory for cluster blas
                RHI::Ptr<RHI::DeviceBuffer> m_clusterBlasBuffer;
                uint64_t m_clusterBlasBufferBufferDeviceAddress;

                // memory for all CLASes within this cluster blas
                RHI::Ptr<RHI::DeviceBuffer> m_dstImplicitDataBuffer;
                uint64_t m_dstImplicitDataBufferDeviceAddress;

                RHI::Ptr<RHI::DeviceBuffer> m_scratchDataBuffer;
                uint64_t m_scratchDataBufferDeviceAddress;

                RHI::Ptr<RHI::DeviceBuffer> m_dstAddressesArrayBuffer;
                uint64_t m_dstAddressesArrayBufferDeviceAddress;
                uint32_t m_dstAddressesArrayBufferSize = 0;
                uint32_t m_dstAddressesArrayBufferStride = 0;

                RHI::Ptr<RHI::DeviceBuffer> m_dstSizesArrayBuffer;
                uint64_t m_dstSizesArrayBufferDeviceAddress;
                uint32_t m_dstSizesArrayBufferSize = 0;
                uint32_t m_dstSizesArrayBufferStride = 0;

                RHI::Ptr<RHI::DeviceBuffer> m_srcInfosArrayBuffer;
                RHI::Ptr<RHI::DeviceBuffer> m_blasSrcInfosBuffer;
                uint64_t m_srcInfosArrayBufferDeviceAddress;
                uint32_t m_srcInfosArrayBufferSize = 0;
                uint32_t m_srcInfosArrayBufferStride = 0;

                RHI::Ptr<RHI::DeviceBuffer> m_srcInfosCountBuffer;
                uint64_t m_srcInfosCountBufferDeviceAddress;

                VkClusterAccelerationStructureClustersBottomLevelInputNV m_ClustersBottomLevelInput;
                VkClusterAccelerationStructureTriangleClusterInputNV m_TriangleClustersInput;
                VkClusterAccelerationStructureMoveObjectsInputNV m_MoveObjectsInput;

                VkClusterAccelerationStructureInputInfoNV m_buildClasesInputInfo;
                VkClusterAccelerationStructureInputInfoNV m_buildClusterBlasInputInfo;
                VkClusterAccelerationStructureInputInfoNV m_copyClasesInputInfo;

                VkClusterAccelerationStructureCommandsInfoNV m_commandInfo = {};
            };

            const ClusterBlasBuffers& GetBuffers() const { return m_buffers.GetCurrentElement(); }

            const RHI::Ptr<RHI::DeviceBuffer> GetDstAddressesArrayBuffer() const override { return GetBuffers().m_dstAddressesArrayBuffer; }
            const RHI::Ptr<RHI::DeviceBuffer> GetDstSizesArrayBuffer() const override { return GetBuffers().m_dstSizesArrayBuffer; }
            const RHI::Ptr<RHI::DeviceBuffer> GetSrcInfosArrayBuffer() const override { return GetBuffers().m_srcInfosArrayBuffer; }
            const RHI::Ptr<RHI::DeviceBuffer> GetSrcInfosCountBuffer() const override { return GetBuffers().m_srcInfosCountBuffer; }

            // RHI::DeviceRayTracingClusterBlas overrides...
            //virtual bool IsValid() const override { return GetBuffers().m_accelerationStructure != VK_NULL_HANDLE; }
            void PrepareBuildClases() override;

            void PrepareBuildClusterBlas() override;

            void PrepareCopyClases() override;

            uint64_t GetAccelerationStructureByteSize() override;

        private:
            RayTracingClusterBlas() = default;

            // RHI::DeviceRayTracingClusterBlas overrides...
            RHI::ResultCode CreateBuffersInternal(RHI::Device& deviceBase, const RHI::DeviceRayTracingClusterBlasDescriptor* descriptor, const RHI::DeviceRayTracingBufferPools& rayTracingBufferPools) override;

            static VkBuildAccelerationStructureFlagsKHR GetAccelerationStructureBuildFlags(const RHI::RayTracingAccelerationStructureBuildFlags &buildFlags);

            //RHI::ResultCode ReleaseScratchBufferInternal(uint64_t frameCount);
            // buffer list to keep buffers alive for several frames
            RHI::FrameCountMaxRingBuffer<ClusterBlasBuffers> m_buffers;
        };
    }
}
