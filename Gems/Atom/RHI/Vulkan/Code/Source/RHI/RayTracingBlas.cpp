/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/RayTracingBlas.h>
#include <RHI/Buffer.h>
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <RHI/Device.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/DeviceBufferPool.h>
#include <Atom/RHI/DeviceRayTracingBufferPools.h>
#include <Atom/RHI.Reflect/VkAllocator.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<RayTracingBlas> RayTracingBlas::Create()
        {
            return aznew RayTracingBlas;
        }

        RHI::ResultCode RayTracingBlas::CreateBuffersInternal(RHI::Device& deviceBase, const RHI::DeviceRayTracingBlasDescriptor* descriptor, const RHI::DeviceRayTracingBufferPools& bufferPools)
        {
            auto& device = static_cast<Device&>(deviceBase);
            auto& physicalDevice = static_cast<const PhysicalDevice&>(device.GetPhysicalDevice());
            const VkPhysicalDeviceAccelerationStructurePropertiesKHR& accelerationStructureProperties = physicalDevice.GetPhysicalDeviceAccelerationStructureProperties();

            // advance to the next buffer
            BlasBuffers& buffers = m_buffers.AdvanceCurrentElement();

            if (buffers.m_accelerationStructure)
            {
                device.GetContext().DestroyAccelerationStructureKHR(
                    device.GetNativeDevice(), buffers.m_accelerationStructure, VkSystemAllocator::Get());
                buffers.m_accelerationStructure = nullptr;
            }

            // build the list of VkAccelerationStructureGeometryKHR structures
            buffers.m_geometryDescs.clear();
            buffers.m_rangeInfos.clear();
            AZStd::vector<uint32_t> primitiveCounts;

            // A BLAS can contain either triangle geometry or procedural geometry; decide based on the descriptor which one to create
            if (descriptor->HasAABB())
            {
                const AZ::Aabb& aabb = descriptor->GetAABB();
                buffers.m_aabbBuffer = RHI::Factory::Get().CreateBuffer();
                AZ::RHI::BufferDescriptor blasBufferDescriptor;
                blasBufferDescriptor.m_bindFlags = RHI::BufferBindFlags::CopyRead | RHI::BufferBindFlags::RayTracingAccelerationStructure;
                blasBufferDescriptor.m_byteCount = sizeof(VkAabbPositionsKHR);
                blasBufferDescriptor.m_alignment = RHI::AlignUp(sizeof(VkAabbPositionsKHR), 8);

                VkAabbPositionsKHR rtAabb;
                rtAabb.minX = aabb.GetMin().GetX();
                rtAabb.minY = aabb.GetMin().GetY();
                rtAabb.minZ = aabb.GetMin().GetZ();
                rtAabb.maxX = aabb.GetMax().GetX();
                rtAabb.maxY = aabb.GetMax().GetY();
                rtAabb.maxZ = aabb.GetMax().GetZ();

                AZ::RHI::DeviceBufferInitRequest blasBufferRequest;
                blasBufferRequest.m_buffer = buffers.m_aabbBuffer.get();
                blasBufferRequest.m_initialData = &rtAabb;
                blasBufferRequest.m_descriptor = blasBufferDescriptor;
                auto resultCode = bufferPools.GetAabbStagingBufferPool()->InitBuffer(blasBufferRequest);
                if (resultCode != AZ::RHI::ResultCode::Success)
                {
                    AZ_Error("RayTracing", false, "Failed to initialize BLAS buffer index buffer with error code: %d", resultCode);
                }

                VkBufferDeviceAddressInfo addressInfo = {};
                addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
                addressInfo.pNext = nullptr;
                addressInfo.buffer = static_cast<const Vulkan::Buffer*>(buffers.m_aabbBuffer.get())->GetBufferMemoryView()->GetNativeBuffer();

                VkAccelerationStructureGeometryKHR geometryDesc = {};
                geometryDesc.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
                geometryDesc.pNext = nullptr;
                geometryDesc.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
                geometryDesc.geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
                geometryDesc.geometry.aabbs.pNext = nullptr;
                geometryDesc.geometry.aabbs.data.deviceAddress = device.GetContext().GetBufferDeviceAddress(device.GetNativeDevice(), &addressInfo);
                geometryDesc.geometry.aabbs.stride = RHI::AlignUp(sizeof(VkAabbPositionsKHR), 8);
                geometryDesc.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

                buffers.m_geometryDescs.push_back(geometryDesc);
                primitiveCounts.push_back(1);

                VkAccelerationStructureBuildRangeInfoKHR rangeInfo = {};
                rangeInfo.firstVertex = 0;
                rangeInfo.primitiveOffset = 0;
                rangeInfo.primitiveCount = 1;
                rangeInfo.transformOffset = 0;
                buffers.m_rangeInfos.push_back(rangeInfo);
            }
            else
            {
                const RHI::DeviceRayTracingGeometryVector& geometries = descriptor->GetGeometries();

                buffers.m_geometryDescs.reserve(geometries.size());
                buffers.m_rangeInfos.reserve(geometries.size());
                primitiveCounts.reserve(geometries.size());

                for (const RHI::DeviceRayTracingGeometry& geometry : geometries)
                {
                    VkAccelerationStructureGeometryKHR geometryDesc = {};
                    geometryDesc.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
                    geometryDesc.pNext = nullptr;
                    geometryDesc.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
                    geometryDesc.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
                    geometryDesc.geometry.triangles.pNext = nullptr;

                    VkBufferDeviceAddressInfo addressInfo = {};
                    addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
                    addressInfo.pNext = nullptr;
                    addressInfo.buffer = static_cast<const Vulkan::Buffer* > (geometry.m_vertexBuffer.GetBuffer())->GetBufferMemoryView()->GetNativeBuffer();
                    geometryDesc.geometry.triangles.vertexData.deviceAddress =
                        device.GetContext().GetBufferDeviceAddress(device.GetNativeDevice(), &addressInfo) +
                        geometry.m_vertexBuffer.GetByteOffset();
                    geometryDesc.geometry.triangles.vertexStride = geometry.m_vertexBuffer.GetByteStride();
                    geometryDesc.geometry.triangles.maxVertex = geometry.m_vertexBuffer.GetByteCount() / aznumeric_cast<uint32_t>(geometryDesc.geometry.triangles.vertexStride);
                    geometryDesc.geometry.triangles.vertexFormat = ConvertFormat(geometry.m_vertexFormat);

                    addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
                    addressInfo.pNext = nullptr;
                    addressInfo.buffer = static_cast<const Vulkan::Buffer*>(geometry.m_indexBuffer.GetBuffer())->GetBufferMemoryView()->GetNativeBuffer();
                    geometryDesc.geometry.triangles.indexData.deviceAddress =
                        device.GetContext().GetBufferDeviceAddress(device.GetNativeDevice(), &addressInfo) +
                        geometry.m_indexBuffer.GetByteOffset();
                    geometryDesc.geometry.triangles.indexType = (geometry.m_indexBuffer.GetIndexFormat() == RHI::IndexFormat::Uint16) ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
                    geometryDesc.geometry.triangles.transformData = {}; // [GFX-TODO][ATOM-4989] Add BLAS Transform Buffer

                    // all BLAS geometry is set to opaque, but can be set to transparent at the TLAS Instance level
                    geometryDesc.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
                    buffers.m_geometryDescs.push_back(geometryDesc);

                    uint32_t indexCount = geometry.m_indexBuffer.GetByteCount() / ((geometry.m_indexBuffer.GetIndexFormat() == RHI::IndexFormat::Uint16) ? 2 : 4);

                    VkAccelerationStructureBuildRangeInfoKHR rangeInfo = {};
                    rangeInfo.firstVertex = 0;
                    rangeInfo.primitiveOffset = 0;
                    rangeInfo.primitiveCount = indexCount / 3;
                    rangeInfo.transformOffset = 0;
                    buffers.m_rangeInfos.push_back(rangeInfo);

                    primitiveCounts.push_back(rangeInfo.primitiveCount);
                }
            }

            buffers.m_buildInfo = {};
            buffers.m_buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
            buffers.m_buildInfo.flags = GetAccelerationStructureBuildFlags(descriptor->GetBuildFlags());
            buffers.m_buildInfo.geometryCount = aznumeric_cast<uint32_t>(buffers.m_geometryDescs.size());
            buffers.m_buildInfo.pGeometries = buffers.m_geometryDescs.data();
            buffers.m_buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
            buffers.m_buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
            buffers.m_buildInfo.srcAccelerationStructure = nullptr;

            VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo = {};
            buildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

            device.GetContext().GetAccelerationStructureBuildSizesKHR(
                device.GetNativeDevice(),
                VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                &buffers.m_buildInfo,
                primitiveCounts.data(),
                &buildSizesInfo);

            buildSizesInfo.accelerationStructureSize = RHI::AlignUp(buildSizesInfo.accelerationStructureSize, 256);
            buildSizesInfo.buildScratchSize = RHI::AlignUp(buildSizesInfo.buildScratchSize, accelerationStructureProperties.minAccelerationStructureScratchOffsetAlignment);

            // create scratch buffer
            buffers.m_scratchBuffer = RHI::Factory::Get().CreateBuffer();
            AZ::RHI::BufferDescriptor scratchBufferDescriptor;
            scratchBufferDescriptor.m_bindFlags = RHI::BufferBindFlags::ShaderReadWrite | RHI::BufferBindFlags::RayTracingScratchBuffer;
            scratchBufferDescriptor.m_byteCount = buildSizesInfo.buildScratchSize;
            scratchBufferDescriptor.m_alignment = accelerationStructureProperties.minAccelerationStructureScratchOffsetAlignment;
            
            AZ::RHI::DeviceBufferInitRequest scratchBufferRequest;
            scratchBufferRequest.m_buffer = buffers.m_scratchBuffer.get();
            scratchBufferRequest.m_descriptor = scratchBufferDescriptor;
            [[maybe_unused]] RHI::ResultCode resultCode = bufferPools.GetScratchBufferPool()->InitBuffer(scratchBufferRequest);
            AZ_Assert(resultCode == RHI::ResultCode::Success, "failed to create BLAS scratch buffer");
            
            BufferMemoryView* scratchMemoryView = static_cast<Buffer*>(buffers.m_scratchBuffer.get())->GetBufferMemoryView();
            scratchMemoryView->SetName("BLAS Scratch");

            // create BLAS buffer
            buffers.m_blasBuffer = RHI::Factory::Get().CreateBuffer();
            AZ::RHI::BufferDescriptor blasBufferDescriptor;
            blasBufferDescriptor.m_bindFlags = RHI::BufferBindFlags::ShaderReadWrite | RHI::BufferBindFlags::RayTracingAccelerationStructure;
            blasBufferDescriptor.m_byteCount = buildSizesInfo.accelerationStructureSize;
            
            AZ::RHI::DeviceBufferInitRequest blasBufferRequest;
            blasBufferRequest.m_buffer = buffers.m_blasBuffer.get();
            blasBufferRequest.m_descriptor = blasBufferDescriptor;
            resultCode = bufferPools.GetBlasBufferPool()->InitBuffer(blasBufferRequest);
            AZ_Assert(resultCode == RHI::ResultCode::Success, "failed to create BLAS buffer");
            
            BufferMemoryView* blasMemoryView = static_cast<Buffer*>(buffers.m_blasBuffer.get())->GetBufferMemoryView();
            blasMemoryView->SetName("BLAS");

            // create BLAS
            VkAccelerationStructureCreateInfoKHR createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
            createInfo.pNext = nullptr;
            createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
            createInfo.size = buildSizesInfo.accelerationStructureSize;
            createInfo.offset = 0;

            VkBufferDeviceAddressInfo addressInfo = {};
            addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
            addressInfo.buffer = static_cast<Buffer*>(buffers.m_blasBuffer.get())->GetBufferMemoryView()->GetNativeBuffer();
            createInfo.buffer = blasMemoryView->GetNativeBuffer();

            VkResult vkResult = device.GetContext().CreateAccelerationStructureKHR(
                device.GetNativeDevice(), &createInfo, VkSystemAllocator::Get(), &buffers.m_accelerationStructure);
            AssertSuccess(vkResult);

            buffers.m_buildInfo.dstAccelerationStructure = buffers.m_accelerationStructure;

            addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
            addressInfo.buffer = scratchMemoryView->GetNativeBuffer();
            buffers.m_buildInfo.scratchData.deviceAddress =
                device.GetContext().GetBufferDeviceAddress(device.GetNativeDevice(), &addressInfo);

            return RHI::ResultCode::Success;
        }

        VkBuildAccelerationStructureFlagsKHR RayTracingBlas::GetAccelerationStructureBuildFlags(const RHI::RayTracingAccelerationStructureBuildFlags &buildFlags)
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

            return vkBuildFlags;
        }
    }
}
