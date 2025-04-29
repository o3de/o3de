/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/VkAllocator.h>
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <Atom/RHI/DeviceBufferPool.h>
#include <Atom/RHI/DeviceRayTracingBufferPools.h>
#include <Atom/RHI/Factory.h>
#include <RHI/Buffer.h>
#include <RHI/Device.h>
#include <RHI/RayTracingAccelerationStructure.h>
#include <RHI/RayTracingClusterBlas.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<RayTracingClusterBlas> RayTracingClusterBlas::Create()
        {
            return aznew RayTracingClusterBlas;
        }

        void RayTracingClusterBlas::PrepareBuildClases()
        {
            ClusterBlasBuffers& buffers = m_buffers.AdvanceCurrentElement();
            const uint32_t clusterCount = buffers.m_ClustersBottomLevelInput.maxTotalClusterCount;

            VkClusterAccelerationStructureInputInfoNV& inputInfo = buffers.m_commandInfo.input;
            inputInfo.maxAccelerationStructureCount = clusterCount;
            inputInfo.opType = VK_CLUSTER_ACCELERATION_STRUCTURE_OP_TYPE_BUILD_TRIANGLE_CLUSTER_NV;
            inputInfo.opMode = VK_CLUSTER_ACCELERATION_STRUCTURE_OP_MODE_IMPLICIT_DESTINATIONS_NV;
            inputInfo.opInput.pTriangleClusters = &buffers.m_TriangleClustersInput;

            VkStridedDeviceAddressRegionKHR& srcInfosArray = buffers.m_commandInfo.srcInfosArray;
            srcInfosArray.stride = sizeof(VkClusterAccelerationStructureBuildTriangleClusterInfoNV);
            srcInfosArray.size = clusterCount * sizeof(VkClusterAccelerationStructureBuildTriangleClusterInfoNV);

            buffers.m_commandInfo.dstImplicitData = buffers.m_dstImplicitDataBufferDeviceAddress;
        }

        void RayTracingClusterBlas::PrepareBuildClusterBlas()
        {
            //Device& device = static_cast<Device&>(GetDevice());
            ClusterBlasBuffers& buffers = m_buffers.AdvanceCurrentElement();

            VkClusterAccelerationStructureInputInfoNV& inputInfo = buffers.m_commandInfo.input;
            inputInfo.maxAccelerationStructureCount = 1;
            inputInfo.opType = VK_CLUSTER_ACCELERATION_STRUCTURE_OP_TYPE_BUILD_CLUSTERS_BOTTOM_LEVEL_NV;
            inputInfo.opMode = VK_CLUSTER_ACCELERATION_STRUCTURE_OP_MODE_EXPLICIT_DESTINATIONS_NV;
            inputInfo.opInput.pClustersBottomLevel = &buffers.m_ClustersBottomLevelInput;

            VkStridedDeviceAddressRegionKHR& srcInfosArray = buffers.m_commandInfo.srcInfosArray;
            srcInfosArray.stride = sizeof(VkClusterAccelerationStructureBuildClustersBottomLevelInfoNV);
            srcInfosArray.size = sizeof(VkClusterAccelerationStructureBuildClustersBottomLevelInfoNV);

            buffers.m_commandInfo.dstImplicitData = buffers.m_clusterBlasBufferBufferDeviceAddress;

            //// fill destination address with cluster BLAS buffer in explicit destination mode
            //RHI::DeviceBufferMapResponse mapResponse;
            //RHI::ResultCode resultCode = bufferPools.GetTlasInstancesBufferPool()->MapBuffer(RHI::DeviceBufferMapRequest(*buffers.m_tlasInstancesBuffer, 0, instanceDescsSizeInBytes), mapResponse);
            //AZ_Assert(resultCode == RHI::ResultCode::Success, "failed to map TLAS instances buffer");
            //VkAccelerationStructureInstanceKHR* mappedData = reinterpret_cast<VkAccelerationStructureInstanceKHR*>(mapResponse.m_data);

            //VkBufferDeviceAddressInfo addressInfo = {};
            //addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
            //addressInfo.pNext = nullptr;
            //addressInfo.buffer = static_cast<Buffer*>(buffers.m_clusterBlasBuffer.get())->GetBufferMemoryView()->GetNativeBuffer();
            //// TODO: is it correct?
            //mappedData[i].accelerationStructureReference = device.GetContext().GetBufferDeviceAddress(device.GetNativeDevice(), &addressInfo);
        }

        void RayTracingClusterBlas::PrepareCopyClases()
        {
            //ClusterBlasBuffers& buffers = m_buffers.AdvanceCurrentElement();

            //// fill m_MoveObjectsInput
            //buffers.m_MoveObjectsInput.sType = VK_STRUCTURE_TYPE_CLUSTER_ACCELERATION_STRUCTURE_MOVE_OBJECTS_INPUT_NV;
            //// TODO: is it too large as it is the size of whole implicit buffer
            //buffers.m_MoveObjectsInput.maxMovedBytes = buildSizesInfo.accelerationStructureSize;
            //buffers.m_MoveObjectsInput.noMoveOverlap = VK_TRUE;

        }

        uint64_t RayTracingClusterBlas::GetAccelerationStructureByteSize()
        {
            return m_buffers.GetCurrentElement().m_clusterBlasBuffer->GetDescriptor().m_byteCount;
        }

        RHI::ResultCode RayTracingClusterBlas::CreateBuffersInternal(RHI::Device& deviceBase, const RHI::DeviceRayTracingClusterBlasDescriptor* descriptor, const RHI::DeviceRayTracingBufferPools& bufferPools)
        {
            auto& device = static_cast<Device&>(deviceBase);
            auto& physicalDevice = static_cast<const PhysicalDevice&>(device.GetPhysicalDevice());
            const VkPhysicalDeviceAccelerationStructurePropertiesKHR& accelerationStructureProperties = physicalDevice.GetPhysicalDeviceAccelerationStructureProperties();

            // advance to the next buffer
            ClusterBlasBuffers& buffers = m_buffers.AdvanceCurrentElement();

            VkBufferDeviceAddressInfo addressInfo = {};
            addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
            addressInfo.pNext = nullptr;

            {
                // fill input for builindg CLASes
                buffers.m_TriangleClustersInput.sType = VK_STRUCTURE_TYPE_CLUSTER_ACCELERATION_STRUCTURE_TRIANGLE_CLUSTER_INPUT_NV;
                buffers.m_TriangleClustersInput.vertexFormat = ConvertFormat(descriptor->GetVertexFormat());
                buffers.m_TriangleClustersInput.maxGeometryIndexValue = descriptor->GetMaxGeometryIndexValue();
                buffers.m_TriangleClustersInput.maxClusterUniqueGeometryCount = descriptor->GetMaxClusterUniqueGeometryCount();
                buffers.m_TriangleClustersInput.maxClusterTriangleCount = descriptor->GetMaxClusterTriangleCount();
                buffers.m_TriangleClustersInput.maxClusterVertexCount = descriptor->GetMaxClusterVertexCount();
                buffers.m_TriangleClustersInput.maxTotalTriangleCount = descriptor->GetMaxTotalTriangleCount();
                buffers.m_TriangleClustersInput.maxTotalVertexCount = descriptor->GetMaxTotalVertexCount();

                // query buffer size for building CLASes with implicit destination mode
                VkClusterAccelerationStructureInputInfoNV& inputInfo = buffers.m_buildClasesInputInfo;
                inputInfo.sType = VK_STRUCTURE_TYPE_CLUSTER_ACCELERATION_STRUCTURE_INPUT_INFO_NV;
                inputInfo.maxAccelerationStructureCount = descriptor->GetMaxClusterCount();
                inputInfo.flags = GetAccelerationStructureBuildFlags(descriptor->GetBuildFlags());
                inputInfo.opType = VK_CLUSTER_ACCELERATION_STRUCTURE_OP_TYPE_BUILD_TRIANGLE_CLUSTER_NV;
                inputInfo.opMode = VK_CLUSTER_ACCELERATION_STRUCTURE_OP_MODE_IMPLICIT_DESTINATIONS_NV;
                inputInfo.opInput.pTriangleClusters = &buffers.m_TriangleClustersInput;

                VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo = {};
                device.GetContext().GetClusterAccelerationStructureBuildSizesNV(
                    device.GetNativeDevice(),
                    &inputInfo,
                    &buildSizesInfo);
                buildSizesInfo.accelerationStructureSize = RHI::AlignUp(buildSizesInfo.accelerationStructureSize, 256);
                buildSizesInfo.buildScratchSize = RHI::AlignUp(buildSizesInfo.buildScratchSize, accelerationStructureProperties.minAccelerationStructureScratchOffsetAlignment);

                // create destination implicit data buffer
                buffers.m_dstImplicitDataBuffer = RHI::Factory::Get().CreateBuffer();
                AZ::RHI::BufferDescriptor implicitBufferDescriptor;
                implicitBufferDescriptor.m_bindFlags = RHI::BufferBindFlags::ShaderReadWrite | RHI::BufferBindFlags::RayTracingAccelerationStructure;
                implicitBufferDescriptor.m_byteCount = buildSizesInfo.accelerationStructureSize;

                AZ::RHI::DeviceBufferInitRequest implicitBufferRequest;
                implicitBufferRequest.m_buffer = buffers.m_dstImplicitDataBuffer.get();
                implicitBufferRequest.m_descriptor = implicitBufferDescriptor;
                RHI::ResultCode resultCode = bufferPools.GetDstImplicitBufferPool()->InitBuffer(implicitBufferRequest);
                AZ_Assert(resultCode == RHI::ResultCode::Success, "failed to create destination implicit buffer");

                addressInfo.buffer = static_cast<const Vulkan::Buffer*>(buffers.m_dstImplicitDataBuffer.get())->GetBufferMemoryView()->GetNativeBuffer();
                buffers.m_dstImplicitDataBufferDeviceAddress = device.GetContext().GetBufferDeviceAddress(device.GetNativeDevice(), &addressInfo);

                // create scratch data buffer
                buffers.m_scratchDataBuffer = RHI::Factory::Get().CreateBuffer();
                AZ::RHI::BufferDescriptor scratchBufferDescriptor;
                scratchBufferDescriptor.m_bindFlags = RHI::BufferBindFlags::ShaderReadWrite | RHI::BufferBindFlags::RayTracingScratchBuffer;
                // TODO: check whether the size is enough also for other operations
                scratchBufferDescriptor.m_byteCount = buildSizesInfo.buildScratchSize;
                scratchBufferDescriptor.m_alignment = accelerationStructureProperties.minAccelerationStructureScratchOffsetAlignment;

                AZ::RHI::DeviceBufferInitRequest scratchBufferRequest;
                scratchBufferRequest.m_buffer = buffers.m_scratchDataBuffer.get();
                scratchBufferRequest.m_descriptor = scratchBufferDescriptor;
                resultCode = bufferPools.GetScratchBufferPool()->InitBuffer(scratchBufferRequest);
                AZ_Assert(resultCode == RHI::ResultCode::Success, "failed to create Cluster BLAS scratch buffer");

                addressInfo.buffer = static_cast<const Vulkan::Buffer*>(buffers.m_scratchDataBuffer.get())->GetBufferMemoryView()->GetNativeBuffer();
                buffers.m_scratchDataBufferDeviceAddress = device.GetContext().GetBufferDeviceAddress(device.GetNativeDevice(), &addressInfo);
            }

            {
                // fill input for builindg cluster BLAS
                buffers.m_ClustersBottomLevelInput.sType = VK_STRUCTURE_TYPE_CLUSTER_ACCELERATION_STRUCTURE_CLUSTERS_BOTTOM_LEVEL_INPUT_NV;
                buffers.m_ClustersBottomLevelInput.maxClusterCountPerAccelerationStructure = descriptor->GetMaxClusterCount();
                buffers.m_ClustersBottomLevelInput.maxTotalClusterCount = descriptor->GetMaxClusterCount();

                // query buffer size for building cluster BLAS with implicit destination mode
                VkClusterAccelerationStructureInputInfoNV& inputInfo = buffers.m_buildClusterBlasInputInfo;
                inputInfo.sType = VK_STRUCTURE_TYPE_CLUSTER_ACCELERATION_STRUCTURE_INPUT_INFO_NV;
                inputInfo.maxAccelerationStructureCount = 1;
                inputInfo.flags = GetAccelerationStructureBuildFlags(descriptor->GetBuildFlags());
                inputInfo.opType = VK_CLUSTER_ACCELERATION_STRUCTURE_OP_TYPE_BUILD_CLUSTERS_BOTTOM_LEVEL_NV;
                inputInfo.opMode = VK_CLUSTER_ACCELERATION_STRUCTURE_OP_MODE_IMPLICIT_DESTINATIONS_NV;
                inputInfo.opInput.pClustersBottomLevel = &buffers.m_ClustersBottomLevelInput;

                VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo = {};
                device.GetContext().GetClusterAccelerationStructureBuildSizesNV(
                    device.GetNativeDevice(),
                    &inputInfo,
                    &buildSizesInfo);
                buildSizesInfo.accelerationStructureSize = RHI::AlignUp(buildSizesInfo.accelerationStructureSize, 256);
                buildSizesInfo.buildScratchSize = RHI::AlignUp(buildSizesInfo.buildScratchSize, accelerationStructureProperties.minAccelerationStructureScratchOffsetAlignment);

                // create cluster blas buffer
                buffers.m_clusterBlasBuffer = RHI::Factory::Get().CreateBuffer();
                AZ::RHI::BufferDescriptor clusterBlasBufferDescriptor;
                clusterBlasBufferDescriptor.m_bindFlags = RHI::BufferBindFlags::ShaderReadWrite | RHI::BufferBindFlags::RayTracingAccelerationStructure;
                clusterBlasBufferDescriptor.m_byteCount = buildSizesInfo.accelerationStructureSize;

                AZ::RHI::DeviceBufferInitRequest clusterBlasBufferRequest;
                clusterBlasBufferRequest.m_buffer = buffers.m_clusterBlasBuffer.get();
                clusterBlasBufferRequest.m_descriptor = clusterBlasBufferDescriptor;
                RHI::ResultCode resultCode = bufferPools.GetBlasBufferPool()->InitBuffer(clusterBlasBufferRequest);
                AZ_Assert(resultCode == RHI::ResultCode::Success, "failed to create cluster blas buffer");

                addressInfo.buffer = static_cast<const Vulkan::Buffer*>(buffers.m_clusterBlasBuffer.get())->GetBufferMemoryView()->GetNativeBuffer();
                buffers.m_clusterBlasBufferBufferDeviceAddress = device.GetContext().GetBufferDeviceAddress(device.GetNativeDevice(), &addressInfo);
            }

            // create destination addresses array buffer
            buffers.m_dstAddressesArrayBuffer = RHI::Factory::Get().CreateBuffer();
            buffers.m_dstAddressesArrayBufferSize = descriptor->GetMaxClusterCount() * sizeof(uint64_t);
            buffers.m_dstAddressesArrayBufferStride = sizeof(uint64_t);
            AZ::RHI::BufferDescriptor dstAddressesBufferDescriptor;
            dstAddressesBufferDescriptor.m_bindFlags = RHI::BufferBindFlags::ShaderReadWrite | RHI::BufferBindFlags::RayTracingAccelerationStructure;
            dstAddressesBufferDescriptor.m_byteCount = descriptor->GetMaxClusterCount() * sizeof(uint64_t);

            AZ::RHI::DeviceBufferInitRequest dstAddressesBufferRequest;
            dstAddressesBufferRequest.m_buffer = buffers.m_dstAddressesArrayBuffer.get();
            dstAddressesBufferRequest.m_descriptor = dstAddressesBufferDescriptor;
            RHI::ResultCode resultCode = bufferPools.GetDstAddressesArrayBufferPool()->InitBuffer(dstAddressesBufferRequest);
            AZ_Assert(resultCode == RHI::ResultCode::Success, "failed to create destination addresses buffer");

            addressInfo.buffer = static_cast<const Vulkan::Buffer*>(buffers.m_dstAddressesArrayBuffer.get())->GetBufferMemoryView()->GetNativeBuffer();
            buffers.m_dstAddressesArrayBufferDeviceAddress = device.GetContext().GetBufferDeviceAddress(device.GetNativeDevice(), &addressInfo);

            // create destination sizes array buffer
            buffers.m_dstSizesArrayBuffer = RHI::Factory::Get().CreateBuffer();
            buffers.m_dstSizesArrayBufferSize = descriptor->GetMaxClusterCount() * sizeof(uint32_t);
            buffers.m_dstSizesArrayBufferStride = sizeof(uint32_t);
            AZ::RHI::BufferDescriptor dstSizesBufferDescriptor;
            dstSizesBufferDescriptor.m_bindFlags = RHI::BufferBindFlags::ShaderReadWrite | RHI::BufferBindFlags::RayTracingAccelerationStructure;
            dstSizesBufferDescriptor.m_byteCount = descriptor->GetMaxClusterCount() * sizeof(uint32_t);

            AZ::RHI::DeviceBufferInitRequest dstSizesBufferRequest;
            dstSizesBufferRequest.m_buffer = buffers.m_dstSizesArrayBuffer.get();
            dstSizesBufferRequest.m_descriptor = dstSizesBufferDescriptor;
            resultCode = bufferPools.GetDstSizesArrayBufferPool()->InitBuffer(dstSizesBufferRequest);
            AZ_Assert(resultCode == RHI::ResultCode::Success, "failed to create destination sizes buffer");

            addressInfo.buffer = static_cast<const Vulkan::Buffer*>(buffers.m_dstSizesArrayBuffer.get())->GetBufferMemoryView()->GetNativeBuffer();
            buffers.m_dstSizesArrayBufferDeviceAddress = device.GetContext().GetBufferDeviceAddress(device.GetNativeDevice(), &addressInfo);

            // create source info array buffer
            /*union SrcInfoFormat
            {
                VkClusterAccelerationStructureMoveObjectsInfoNV m_moveInfo;
                VkClusterAccelerationStructureBuildClustersBottomLevelInfoNV m_clusterBlasInfo;
                VkClusterAccelerationStructureBuildTriangleClusterInfoNV m_;
            };*/
            buffers.m_srcInfosArrayBuffer = RHI::Factory::Get().CreateBuffer();
            //buffers.m_srcInfosArrayBufferSize = descriptor->GetMaxClusterCount() * sizeof(VkClusterAccelerationStructureBuildTriangleClusterInfoNV);
            //buffers.m_srcInfosArrayBufferStride = sizeof(uint32_t);
            AZ::RHI::BufferDescriptor srcInfosBufferDescriptor;
            srcInfosBufferDescriptor.m_bindFlags = RHI::BufferBindFlags::ShaderReadWrite | RHI::BufferBindFlags::RayTracingAccelerationStructure;
            // TODO: check whether is the max size
            srcInfosBufferDescriptor.m_byteCount = descriptor->GetMaxClusterCount() * sizeof(VkClusterAccelerationStructureBuildTriangleClusterInfoNV);

            AZ::RHI::DeviceBufferInitRequest srcInfosBufferRequest;
            srcInfosBufferRequest.m_buffer = buffers.m_srcInfosArrayBuffer.get();
            srcInfosBufferRequest.m_descriptor = srcInfosBufferDescriptor;
            resultCode = bufferPools.GetSrcInfosArrayBufferPool()->InitBuffer(srcInfosBufferRequest);
            AZ_Assert(resultCode == RHI::ResultCode::Success, "failed to create source infos buffer");

            addressInfo.buffer = static_cast<const Vulkan::Buffer*>(buffers.m_srcInfosArrayBuffer.get())->GetBufferMemoryView()->GetNativeBuffer();
            buffers.m_srcInfosArrayBufferDeviceAddress = device.GetContext().GetBufferDeviceAddress(device.GetNativeDevice(), &addressInfo);

            // create source info count buffer
            buffers.m_srcInfosCountBuffer = RHI::Factory::Get().CreateBuffer();
            AZ::RHI::BufferDescriptor srcInfosCountBufferDescriptor;
            srcInfosCountBufferDescriptor.m_bindFlags = RHI::BufferBindFlags::ShaderReadWrite | RHI::BufferBindFlags::RayTracingAccelerationStructure;
            srcInfosCountBufferDescriptor.m_byteCount = descriptor->GetMaxClusterCount() * sizeof(uint32_t);

            AZ::RHI::DeviceBufferInitRequest srcInfosCountBufferRequest;
            srcInfosCountBufferRequest.m_buffer = buffers.m_srcInfosCountBuffer.get();
            srcInfosCountBufferRequest.m_descriptor = srcInfosCountBufferDescriptor;
            resultCode = bufferPools.GetSrcInfosCountBufferPool()->InitBuffer(srcInfosCountBufferRequest);
            AZ_Assert(resultCode == RHI::ResultCode::Success, "failed to create destination sizes buffer");

            addressInfo.buffer = static_cast<const Vulkan::Buffer*>(buffers.m_srcInfosCountBuffer.get())->GetBufferMemoryView()->GetNativeBuffer();
            buffers.m_srcInfosCountBufferDeviceAddress = device.GetContext().GetBufferDeviceAddress(device.GetNativeDevice(), &addressInfo);

            {
                // assign buffer addresses to command info
                
                buffers.m_commandInfo.sType = VK_STRUCTURE_TYPE_CLUSTER_ACCELERATION_STRUCTURE_COMMANDS_INFO_NV;

                addressInfo.buffer = static_cast<const Vulkan::Buffer*>(buffers.m_dstImplicitDataBuffer.get())->GetBufferMemoryView()->GetNativeBuffer();
                buffers.m_commandInfo.dstImplicitData = device.GetContext().GetBufferDeviceAddress(device.GetNativeDevice(), &addressInfo);

                addressInfo.buffer = static_cast<const Vulkan::Buffer*>(buffers.m_scratchDataBuffer.get())->GetBufferMemoryView()->GetNativeBuffer();
                buffers.m_commandInfo.scratchData = device.GetContext().GetBufferDeviceAddress(device.GetNativeDevice(), &addressInfo);

                VkStridedDeviceAddressRegionKHR dstAddressesArray;
                dstAddressesArray.deviceAddress = buffers.m_dstAddressesArrayBufferDeviceAddress;
                dstAddressesArray.stride = buffers.m_dstAddressesArrayBufferStride;
                dstAddressesArray.size = buffers.m_dstAddressesArrayBufferSize;
                buffers.m_commandInfo.dstAddressesArray = dstAddressesArray;

                VkStridedDeviceAddressRegionKHR dstSizesArray;
                dstSizesArray.deviceAddress = buffers.m_dstSizesArrayBufferDeviceAddress;
                dstSizesArray.stride = buffers.m_dstSizesArrayBufferStride;
                dstSizesArray.size = buffers.m_dstSizesArrayBufferSize;
                buffers.m_commandInfo.dstSizesArray = dstSizesArray;

                VkStridedDeviceAddressRegionKHR srcInfosArray;
                srcInfosArray.deviceAddress = buffers.m_srcInfosArrayBufferDeviceAddress;
                srcInfosArray.stride = buffers.m_srcInfosArrayBufferStride;
                srcInfosArray.size = buffers.m_srcInfosArrayBufferSize;
                buffers.m_commandInfo.srcInfosArray = srcInfosArray;

                addressInfo.buffer = static_cast<const Vulkan::Buffer*>(buffers.m_srcInfosCountBuffer.get())->GetBufferMemoryView()->GetNativeBuffer();
                buffers.m_commandInfo.srcInfosCount = device.GetContext().GetBufferDeviceAddress(device.GetNativeDevice(), &addressInfo);
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

        /*RHI::ResultCode RayTracingClusterBlas::ReleaseScratchBufferInternal([[maybe_unused]] uint64_t frameCount)
        {
            m_buffers.GetCurrentElement().m_scratchDataBuffer = nullptr;
            return RHI::ResultCode::Success;
        }*/
    } // namespace Vulkan
} // namespace AZ
