/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <Atom/RHI/DeviceRayTracingAccelerationStructure.h>
#include <Atom/RHI/DeviceRayTracingBufferPools.h>
#include <Atom/RHI/Factory.h>
#include <RHI/Buffer.h>
#include <RHI/Device.h>
#include <RHI/RayTracingClusterBlas.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<RayTracingClusterBlas> RayTracingClusterBlas::Create()
        {
            return aznew RayTracingClusterBlas;
        }

        uint64_t RayTracingClusterBlas::GetAccelerationStructureByteSize()
        {
            return m_buffers.GetCurrentElement().m_clusterBlasDstImplicitBuffer->GetDescriptor().m_byteCount;
        }

        RHI::ResultCode RayTracingClusterBlas::CreateBuffersInternal(
            RHI::Device& deviceBase,
            const RHI::DeviceRayTracingClusterBlasDescriptor* descriptor,
            const RHI::DeviceRayTracingBufferPools& bufferPools)
        {
            auto& device = static_cast<Device&>(deviceBase);
            auto& physicalDevice = static_cast<const PhysicalDevice&>(device.GetPhysicalDevice());
            const auto& accelerationStructureProperties = physicalDevice.GetPhysicalDeviceAccelerationStructureProperties();
            const auto& clusterAccelerationStructureProperties = physicalDevice.GetPhysicalDeviceClusterAccelerationStructureProperties();

            // Advance to the next buffer
            ClusterBlasBuffers& buffers = m_buffers.AdvanceCurrentElement();

            // Configuration for building CLAS with implicit destination
            VkClusterAccelerationStructureInputInfoNV clasInputInfo = {};
            {
                clasInputInfo.sType = VK_STRUCTURE_TYPE_CLUSTER_ACCELERATION_STRUCTURE_INPUT_INFO_NV;
                clasInputInfo.pNext = nullptr;
                clasInputInfo.maxAccelerationStructureCount = descriptor->m_maxClusterCount;
                clasInputInfo.flags = GetAccelerationStructureBuildFlags(descriptor->m_buildFlags);
                clasInputInfo.opType = VK_CLUSTER_ACCELERATION_STRUCTURE_OP_TYPE_BUILD_TRIANGLE_CLUSTER_NV;
                clasInputInfo.opMode = VK_CLUSTER_ACCELERATION_STRUCTURE_OP_MODE_IMPLICIT_DESTINATIONS_NV;
                clasInputInfo.opInput.pTriangleClusters = &buffers.m_triangleClustersInput;

                // Input data for build CLAS operation
                buffers.m_triangleClustersInput.sType = VK_STRUCTURE_TYPE_CLUSTER_ACCELERATION_STRUCTURE_TRIANGLE_CLUSTER_INPUT_NV;
                buffers.m_triangleClustersInput.pNext = nullptr;
                buffers.m_triangleClustersInput.vertexFormat = ConvertFormat(RHI::ConvertToImageFormat(descriptor->m_vertexFormat));
                buffers.m_triangleClustersInput.maxGeometryIndexValue = descriptor->m_maxGeometryIndexValue;
                buffers.m_triangleClustersInput.maxClusterUniqueGeometryCount = descriptor->m_maxClusterUniqueGeometryCount;
                buffers.m_triangleClustersInput.maxClusterTriangleCount = descriptor->m_maxClusterTriangleCount;
                buffers.m_triangleClustersInput.maxClusterVertexCount = descriptor->m_maxClusterVertexCount;
                buffers.m_triangleClustersInput.maxTotalTriangleCount = descriptor->m_maxTotalTriangleCount;
                buffers.m_triangleClustersInput.maxTotalVertexCount = descriptor->m_maxTotalVertexCount;
                buffers.m_triangleClustersInput.minPositionTruncateBitCount = descriptor->m_minPositionTruncateBitCount;
            }

            // Query buffer size for building CLASes with implicit destination
            VkAccelerationStructureBuildSizesInfoKHR clasBuildSizesInfo = {};
            device.GetContext().GetClusterAccelerationStructureBuildSizesNV(device.GetNativeDevice(), &clasInputInfo, &clasBuildSizesInfo);

            {
                // Create CLAS destination implicit data buffer
                buffers.m_clasDstImplicitBuffer = RHI::Factory::Get().CreateBuffer();
                buffers.m_clasDstImplicitBuffer->SetName(Name("CLAS - Cluster destination implicit data"));

                AZ::RHI::BufferDescriptor clasDstImplicitDescriptor;
                clasDstImplicitDescriptor.m_bindFlags = bufferPools.GetDstImplicitBufferPool()->GetDescriptor().m_bindFlags;
                clasDstImplicitDescriptor.m_byteCount = clasBuildSizesInfo.accelerationStructureSize;
                clasDstImplicitDescriptor.m_alignment = clusterAccelerationStructureProperties.clusterByteAlignment;

                AZ::RHI::DeviceBufferInitRequest clasDstImplicitBufferRequest;
                clasDstImplicitBufferRequest.m_buffer = buffers.m_clasDstImplicitBuffer.get();
                clasDstImplicitBufferRequest.m_descriptor = clasDstImplicitDescriptor;
                [[maybe_unused]] RHI::ResultCode resultCode =
                    bufferPools.GetDstImplicitBufferPool()->InitBuffer(clasDstImplicitBufferRequest);
                AZ_Assert(resultCode == RHI::ResultCode::Success, "Failed to create cluster destination implicit buffer");
            }

            // Configuration for building cluster BLAS with implicit destination
            VkClusterAccelerationStructureInputInfoNV clusterBlasInputInfo = {};
            {
                clusterBlasInputInfo.sType = VK_STRUCTURE_TYPE_CLUSTER_ACCELERATION_STRUCTURE_INPUT_INFO_NV;
                clusterBlasInputInfo.pNext = nullptr;
                clusterBlasInputInfo.maxAccelerationStructureCount = 1;
                clusterBlasInputInfo.flags = GetAccelerationStructureBuildFlags(descriptor->m_buildFlags);
                clusterBlasInputInfo.opType = VK_CLUSTER_ACCELERATION_STRUCTURE_OP_TYPE_BUILD_CLUSTERS_BOTTOM_LEVEL_NV;
                clusterBlasInputInfo.opMode = VK_CLUSTER_ACCELERATION_STRUCTURE_OP_MODE_IMPLICIT_DESTINATIONS_NV;
                clusterBlasInputInfo.opInput.pClustersBottomLevel = &buffers.m_clustersBottomLevelInput;

                // Input data for build cluster BLAS operation
                buffers.m_clustersBottomLevelInput.sType = VK_STRUCTURE_TYPE_CLUSTER_ACCELERATION_STRUCTURE_CLUSTERS_BOTTOM_LEVEL_INPUT_NV;
                buffers.m_clustersBottomLevelInput.pNext = nullptr;
                buffers.m_clustersBottomLevelInput.maxClusterCountPerAccelerationStructure = descriptor->m_maxClusterCount;
                buffers.m_clustersBottomLevelInput.maxTotalClusterCount = descriptor->m_maxClusterCount;
            }

            // Query buffer size for building cluster BLAS with implicit destination
            VkAccelerationStructureBuildSizesInfoKHR clusterBlasBuildSizesInfo = {};
            device.GetContext().GetClusterAccelerationStructureBuildSizesNV(
                device.GetNativeDevice(), &clusterBlasInputInfo, &clusterBlasBuildSizesInfo);

            {
                // Create scratch data buffer
                buffers.m_scratchDataBuffer = RHI::Factory::Get().CreateBuffer();
                buffers.m_scratchDataBuffer->SetName(Name("CLAS - Scratch data buffer"));

                AZ::RHI::BufferDescriptor scratchBufferDescriptor;
                scratchBufferDescriptor.m_bindFlags = bufferPools.GetScratchBufferPool()->GetDescriptor().m_bindFlags;
                scratchBufferDescriptor.m_byteCount =
                    AZStd::max(clasBuildSizesInfo.buildScratchSize, clusterBlasBuildSizesInfo.buildScratchSize);
                scratchBufferDescriptor.m_alignment = AZStd::max(
                    clusterAccelerationStructureProperties.clusterScratchByteAlignment,
                    accelerationStructureProperties.minAccelerationStructureScratchOffsetAlignment);

                AZ::RHI::DeviceBufferInitRequest scratchBufferRequest;
                scratchBufferRequest.m_buffer = buffers.m_scratchDataBuffer.get();
                scratchBufferRequest.m_descriptor = scratchBufferDescriptor;
                [[maybe_unused]] RHI::ResultCode resultCode = bufferPools.GetScratchBufferPool()->InitBuffer(scratchBufferRequest);
                AZ_Assert(resultCode == RHI::ResultCode::Success, "Failed to create scratch buffer");
            }

            {
                // Create cluster BLAS buffer
                buffers.m_clusterBlasDstImplicitBuffer = RHI::Factory::Get().CreateBuffer();
                buffers.m_clusterBlasDstImplicitBuffer->SetName(Name("CLAS - Cluster BLAS destination implicit data"));

                AZ::RHI::BufferDescriptor clusterBlasBufferDescriptor;
                clusterBlasBufferDescriptor.m_bindFlags = bufferPools.GetBlasBufferPool()->GetDescriptor().m_bindFlags;
                clusterBlasBufferDescriptor.m_byteCount = clusterBlasBuildSizesInfo.accelerationStructureSize;

                AZ::RHI::DeviceBufferInitRequest clusterBlasBufferRequest;
                clusterBlasBufferRequest.m_buffer = buffers.m_clusterBlasDstImplicitBuffer.get();
                clusterBlasBufferRequest.m_descriptor = clusterBlasBufferDescriptor;
                [[maybe_unused]] RHI::ResultCode resultCode = bufferPools.GetBlasBufferPool()->InitBuffer(clusterBlasBufferRequest);
                AZ_Assert(resultCode == RHI::ResultCode::Success, "Failed to create cluster BLAS destination implicit buffer");
            }

            {
                // Create destination addresses array buffer
                buffers.m_dstAddressesArrayBuffer = RHI::Factory::Get().CreateBuffer();
                buffers.m_dstAddressesArrayBuffer->SetName(Name("CLAS - Destination addresses array"));

                AZ::RHI::BufferDescriptor dstAddressesBufferDescriptor;
                dstAddressesBufferDescriptor.m_bindFlags = bufferPools.GetDstAddressesArrayBufferPool()->GetDescriptor().m_bindFlags;
                dstAddressesBufferDescriptor.m_byteCount = descriptor->m_maxClusterCount * sizeof(uint64_t);

                AZ::RHI::DeviceBufferInitRequest dstAddressesBufferRequest;
                dstAddressesBufferRequest.m_buffer = buffers.m_dstAddressesArrayBuffer.get();
                dstAddressesBufferRequest.m_descriptor = dstAddressesBufferDescriptor;
                [[maybe_unused]] RHI::ResultCode resultCode =
                    bufferPools.GetDstAddressesArrayBufferPool()->InitBuffer(dstAddressesBufferRequest);
                AZ_Assert(resultCode == RHI::ResultCode::Success, "Failed to create destination addresses array buffer");
            }

            {
                // Create destination sizes array buffer
                buffers.m_dstSizesArrayBuffer = RHI::Factory::Get().CreateBuffer();
                buffers.m_dstSizesArrayBuffer->SetName(Name("CLAS - Destination sizes array"));

                AZ::RHI::BufferDescriptor dstSizesBufferDescriptor;
                dstSizesBufferDescriptor.m_bindFlags = bufferPools.GetDstSizesArrayBufferPool()->GetDescriptor().m_bindFlags;
                dstSizesBufferDescriptor.m_byteCount = descriptor->m_maxClusterCount * sizeof(uint32_t);

                AZ::RHI::DeviceBufferInitRequest dstSizesBufferRequest;
                dstSizesBufferRequest.m_buffer = buffers.m_dstSizesArrayBuffer.get();
                dstSizesBufferRequest.m_descriptor = dstSizesBufferDescriptor;
                [[maybe_unused]] RHI::ResultCode resultCode = bufferPools.GetDstSizesArrayBufferPool()->InitBuffer(dstSizesBufferRequest);
                AZ_Assert(resultCode == RHI::ResultCode::Success, "Failed to create destination sizes array buffer");
            }

            {
                // Create source info buffer for cluster-BLAS
                buffers.m_blasSrcInfosBuffer = RHI::Factory::Get().CreateBuffer();
                buffers.m_blasSrcInfosBuffer->SetName(Name("CLAS - BLAS source infos"));

                AZ::RHI::BufferDescriptor blasSrcInfoBufferDescriptor;
                blasSrcInfoBufferDescriptor.m_bindFlags = bufferPools.GetSrcInfosArrayBufferPool()->GetDescriptor().m_bindFlags;
                blasSrcInfoBufferDescriptor.m_byteCount = sizeof(VkClusterAccelerationStructureBuildClustersBottomLevelInfoNV);

                VkClusterAccelerationStructureBuildClustersBottomLevelInfoNV blasSrcInfoData = {};
                blasSrcInfoData.clusterReferencesCount = descriptor->m_maxClusterCount;
                blasSrcInfoData.clusterReferencesStride = sizeof(uint64_t);
                blasSrcInfoData.clusterReferences = buffers.m_dstAddressesArrayBuffer->GetDeviceAddress();

                AZ::RHI::DeviceBufferInitRequest blasSrcInfoBufferRequest;
                blasSrcInfoBufferRequest.m_buffer = buffers.m_blasSrcInfosBuffer.get();
                blasSrcInfoBufferRequest.m_descriptor = blasSrcInfoBufferDescriptor;
                blasSrcInfoBufferRequest.m_initialData = &blasSrcInfoData;
                [[maybe_unused]] RHI::ResultCode resultCode =
                    bufferPools.GetSrcInfosArrayBufferPool()->InitBuffer(blasSrcInfoBufferRequest);
                AZ_Assert(resultCode == RHI::ResultCode::Success, "Failed to create BLAS source infos buffer");
            }

            {
                // Assign buffer addresses to command info
                VkStridedDeviceAddressRegionKHR dstAddressesArray;
                dstAddressesArray.deviceAddress = buffers.m_dstAddressesArrayBuffer->GetDeviceAddress();
                dstAddressesArray.stride = sizeof(uint64_t);
                dstAddressesArray.size = descriptor->m_maxClusterCount * sizeof(uint64_t);

                VkStridedDeviceAddressRegionKHR dstSizesArray;
                dstSizesArray.deviceAddress = buffers.m_dstSizesArrayBuffer->GetDeviceAddress();
                dstSizesArray.stride = sizeof(uint32_t);
                dstSizesArray.size = descriptor->m_maxClusterCount * sizeof(uint32_t);

                {
                    VkStridedDeviceAddressRegionKHR srcInfosArray;
                    srcInfosArray.deviceAddress = descriptor->m_srcInfosArrayBufferView->GetDeviceAddress();
                    srcInfosArray.stride = sizeof(VkClusterAccelerationStructureBuildTriangleClusterInfoNV);
                    srcInfosArray.size = descriptor->m_maxClusterCount * sizeof(VkClusterAccelerationStructureBuildTriangleClusterInfoNV);

                    buffers.m_buildClasCommandInfo.sType = VK_STRUCTURE_TYPE_CLUSTER_ACCELERATION_STRUCTURE_COMMANDS_INFO_NV;
                    buffers.m_buildClasCommandInfo.pNext = nullptr;
                    buffers.m_buildClasCommandInfo.input = clasInputInfo;
                    buffers.m_buildClasCommandInfo.dstImplicitData = buffers.m_clasDstImplicitBuffer->GetDeviceAddress();
                    buffers.m_buildClasCommandInfo.scratchData = buffers.m_scratchDataBuffer->GetDeviceAddress();
                    buffers.m_buildClasCommandInfo.dstAddressesArray = dstAddressesArray;
                    buffers.m_buildClasCommandInfo.dstSizesArray = dstSizesArray;
                    buffers.m_buildClasCommandInfo.srcInfosArray = srcInfosArray;
                    buffers.m_buildClasCommandInfo.srcInfosCount =
                        descriptor->m_srcInfosCountBufferView ? descriptor->m_srcInfosCountBufferView->GetDeviceAddress() : 0;
                }

                {
                    VkStridedDeviceAddressRegionKHR srcInfosArray;
                    srcInfosArray.deviceAddress = buffers.m_blasSrcInfosBuffer->GetDeviceAddress();
                    srcInfosArray.stride = sizeof(VkClusterAccelerationStructureBuildClustersBottomLevelInfoNV);
                    srcInfosArray.size = sizeof(VkClusterAccelerationStructureBuildClustersBottomLevelInfoNV);

                    buffers.m_buildClusterBlasCommandInfo.sType = VK_STRUCTURE_TYPE_CLUSTER_ACCELERATION_STRUCTURE_COMMANDS_INFO_NV;
                    buffers.m_buildClusterBlasCommandInfo.pNext = nullptr;
                    buffers.m_buildClusterBlasCommandInfo.input = clusterBlasInputInfo;
                    buffers.m_buildClusterBlasCommandInfo.dstImplicitData = buffers.m_clusterBlasDstImplicitBuffer->GetDeviceAddress();
                    buffers.m_buildClusterBlasCommandInfo.scratchData = buffers.m_scratchDataBuffer->GetDeviceAddress();
                    buffers.m_buildClusterBlasCommandInfo.dstAddressesArray = dstAddressesArray;
                    buffers.m_buildClusterBlasCommandInfo.dstSizesArray = dstSizesArray;
                    buffers.m_buildClusterBlasCommandInfo.srcInfosArray = srcInfosArray;
                    buffers.m_buildClusterBlasCommandInfo.srcInfosCount =
                        descriptor->m_srcInfosCountBufferView ? descriptor->m_srcInfosCountBufferView->GetDeviceAddress() : 0;
                }
            }

            return RHI::ResultCode::Success;
        }

        VkBuildAccelerationStructureFlagsKHR RayTracingClusterBlas::GetAccelerationStructureBuildFlags(
            const RHI::RayTracingAccelerationStructureBuildFlags& buildFlags)
        {
            VkBuildAccelerationStructureFlagsKHR vkBuildFlags = { 0 };
            if (RHI::CheckBitsAny(buildFlags, RHI::RayTracingAccelerationStructureBuildFlags::FAST_TRACE))
            {
                vkBuildFlags |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
            }

            if (RHI::CheckBitsAny(buildFlags, RHI::RayTracingAccelerationStructureBuildFlags::FAST_BUILD))
            {
                vkBuildFlags |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
            }

            if (RHI::CheckBitsAny(buildFlags, RHI::RayTracingAccelerationStructureBuildFlags::ENABLE_UPDATE))
            {
                vkBuildFlags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
            }

            if (RHI::CheckBitsAny(buildFlags, RHI::RayTracingAccelerationStructureBuildFlags::ENABLE_COMPACTION))
            {
                vkBuildFlags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
            }

            return vkBuildFlags;
        }
    } // namespace Vulkan
} // namespace AZ
