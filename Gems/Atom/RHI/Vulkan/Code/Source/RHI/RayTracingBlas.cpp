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
#include <Atom/RHI/BufferPool.h>
#include <Atom/RHI/RayTracingBufferPools.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<RayTracingBlas> RayTracingBlas::Create()
        {
            return aznew RayTracingBlas;
        }

        RHI::ResultCode RayTracingBlas::CreateBuffersInternal(RHI::Device& deviceBase, const RHI::RayTracingBlasDescriptor* descriptor, const RHI::RayTracingBufferPools& bufferPools)
        {
            auto& device = static_cast<Device&>(deviceBase);
            auto& physicalDevice = static_cast<const PhysicalDevice&>(device.GetPhysicalDevice());
            const VkPhysicalDeviceAccelerationStructurePropertiesKHR& accelerationStructureProperties = physicalDevice.GetPhysicalDeviceAccelerationStructureProperties();

            // advance to the next buffer
            m_currentBufferIndex = (m_currentBufferIndex + 1) % BufferCount;
            BlasBuffers& buffers = m_buffers[m_currentBufferIndex];

            if (buffers.m_accelerationStructure)
            {
                device.GetContext().DestroyAccelerationStructureKHR(device.GetNativeDevice(), buffers.m_accelerationStructure, nullptr);
                buffers.m_accelerationStructure = nullptr;
            }

            const RHI::RayTracingGeometryVector& geometries = descriptor->GetGeometries();

            // build the list of VkAccelerationStructureGeometryKHR structures
            buffers.m_geometryDescs.clear();
            buffers.m_geometryDescs.reserve(geometries.size());
            buffers.m_rangeInfos.clear();
            buffers.m_rangeInfos.reserve(geometries.size());
            AZStd::vector<uint32_t> primitiveCounts;
            primitiveCounts.reserve(geometries.size());

            for (const RHI::RayTracingGeometry& geometry : geometries)
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

            buffers.m_buildInfo = {};
            buffers.m_buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
            buffers.m_buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
            buffers.m_buildInfo.geometryCount = aznumeric_cast<uint32_t>(geometries.size());
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
            
            AZ::RHI::BufferInitRequest scratchBufferRequest;
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
            
            AZ::RHI::BufferInitRequest blasBufferRequest;
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
                device.GetNativeDevice(), &createInfo, nullptr, &buffers.m_accelerationStructure);
            AssertSuccess(vkResult);

            buffers.m_buildInfo.dstAccelerationStructure = buffers.m_accelerationStructure;

            addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
            addressInfo.buffer = scratchMemoryView->GetNativeBuffer();
            buffers.m_buildInfo.scratchData.deviceAddress =
                device.GetContext().GetBufferDeviceAddress(device.GetNativeDevice(), &addressInfo);

            return RHI::ResultCode::Success;
        }
    }
}
