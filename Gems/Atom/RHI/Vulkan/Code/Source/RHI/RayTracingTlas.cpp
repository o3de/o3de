/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/VkAllocator.h>
#include <Atom/RHI/DeviceBufferPool.h>
#include <Atom/RHI/DeviceRayTracingBufferPools.h>
#include <Atom/RHI/Factory.h>
#include <AzCore/Math/Matrix3x4.h>
#include <RHI/Buffer.h>
#include <RHI/BufferView.h>
#include <RHI/Device.h>
#include <RHI/RayTracingAccelerationStructure.h>
#include <RHI/RayTracingBlas.h>
#include <RHI/RayTracingTlas.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<RayTracingTlas> RayTracingTlas::Create()
        {
            return aznew RayTracingTlas;
        }

        RHI::ResultCode RayTracingTlas::CreateBuffersInternal(RHI::Device& deviceBase, const RHI::DeviceRayTracingTlasDescriptor* descriptor, const RHI::DeviceRayTracingBufferPools& bufferPools)
        {
            auto& device = static_cast<Device&>(deviceBase);
            auto& physicalDevice = static_cast<const PhysicalDevice&>(device.GetPhysicalDevice());
            const VkPhysicalDeviceAccelerationStructurePropertiesKHR& accelerationStructureProperties = physicalDevice.GetPhysicalDeviceAccelerationStructureProperties();
                        
            // advance to the next buffer
            TlasBuffers& buffers = m_buffers.AdvanceCurrentElement();

            if (buffers.m_accelerationStructure)
            {
                buffers.m_accelerationStructure = nullptr;
            }

            const RHI::DeviceRayTracingTlasInstanceVector& instances = descriptor->GetInstances();
            if (instances.empty())
            {
                // no instances in the scene, clear the TLAS buffers
                buffers.m_tlasBuffer = nullptr;
                buffers.m_tlasInstancesBuffer = nullptr;
                buffers.m_scratchBuffer = nullptr;
                return RHI::ResultCode::Success;
            }

            AZStd::vector<RHI::Ptr<RHI::DeviceBuffer>> blasBuffers;
            VkDeviceAddress tlasInstancesGpuAddress = 0;
            if (descriptor->GetInstancesBuffer() == nullptr)
            {
                buffers.m_instanceCount = aznumeric_caster(instances.size());
                uint64_t instanceDescsSizeInBytes = aznumeric_cast<uint32_t>(sizeof(VkAccelerationStructureInstanceKHR) * instances.size());
            
                // create instances buffer
                buffers.m_tlasInstancesBuffer = RHI::Factory::Get().CreateBuffer();
                AZ::RHI::BufferDescriptor tlasInstancesBufferDescriptor;
                tlasInstancesBufferDescriptor.m_bindFlags = RHI::BufferBindFlags::ShaderReadWrite | RHI::BufferBindFlags::RayTracingAccelerationStructure;
                tlasInstancesBufferDescriptor.m_byteCount = instanceDescsSizeInBytes;
                
                AZ::RHI::DeviceBufferInitRequest tlasInstancesBufferRequest;
                tlasInstancesBufferRequest.m_buffer = buffers.m_tlasInstancesBuffer.get();
                tlasInstancesBufferRequest.m_descriptor = tlasInstancesBufferDescriptor;
                [[maybe_unused]] RHI::ResultCode resultCode = bufferPools.GetTlasInstancesBufferPool()->InitBuffer(tlasInstancesBufferRequest);
                AZ_Assert(resultCode == RHI::ResultCode::Success, "failed to create TLAS instances buffer");
                
                BufferMemoryView* tlasInstancesMemoryView = static_cast<Buffer*>(buffers.m_tlasInstancesBuffer.get())->GetBufferMemoryView();
                tlasInstancesMemoryView->SetName("TLAS Instance");
                
                RHI::DeviceBufferMapResponse mapResponse;
                resultCode = bufferPools.GetTlasInstancesBufferPool()->MapBuffer(RHI::DeviceBufferMapRequest(*buffers.m_tlasInstancesBuffer, 0, instanceDescsSizeInBytes), mapResponse);
                AZ_Assert(resultCode == RHI::ResultCode::Success, "failed to map TLAS instances buffer");
                VkAccelerationStructureInstanceKHR* mappedData = reinterpret_cast<VkAccelerationStructureInstanceKHR*>(mapResponse.m_data);

                memset(mappedData, 0, instanceDescsSizeInBytes);

                // create each VkAccelerationStructureInstanceKHR structure
                for (uint32_t i = 0; i < instances.size(); ++i)
                {
                    const RHI::DeviceRayTracingTlasInstance& instance = instances[i];
            
                    mappedData[i].instanceCustomIndex = instance.m_instanceID;
                    mappedData[i].instanceShaderBindingTableRecordOffset = instance.m_hitGroupIndex;
                    AZ::Matrix3x4 matrix3x4 = AZ::Matrix3x4::CreateFromTransform(instance.m_transform);
                    matrix3x4.MultiplyByScale(instance.m_nonUniformScale);
                    matrix3x4.StoreToRowMajorFloat12(&mappedData[i].transform.matrix[0][0]);
            
                    RayTracingBlas* blas = static_cast<RayTracingBlas*>(instance.m_blas.get());
                    VkAccelerationStructureDeviceAddressInfoKHR addressInfo = {};
                    addressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
                    addressInfo.pNext = nullptr;
                    addressInfo.accelerationStructure = blas->GetBuffers().m_accelerationStructure->GetNativeAccelerationStructure();
                    mappedData[i].accelerationStructureReference =
                        device.GetContext().GetAccelerationStructureDeviceAddressKHR(device.GetNativeDevice(), &addressInfo);

                    mappedData[i].mask = instance.m_instanceMask;
                    mappedData[i].flags = instance.m_transparent ? VK_GEOMETRY_INSTANCE_FORCE_NO_OPAQUE_BIT_KHR : 0;

                    blasBuffers.emplace_back(blas->GetBuffers().m_blasBuffer);
                }
            
                bufferPools.GetTlasInstancesBufferPool()->UnmapBuffer(*buffers.m_tlasInstancesBuffer);
            
                VkBufferDeviceAddressInfo addressInfo = {};
                addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
                addressInfo.pNext = nullptr;
                addressInfo.buffer = tlasInstancesMemoryView->GetNativeBuffer();
                tlasInstancesGpuAddress = device.GetContext().GetBufferDeviceAddress(device.GetNativeDevice(), &addressInfo);
            }
            else
            {
                AZ_Assert(descriptor->GetNumInstancesInBuffer(), "TLAS InstancesBuffer set but instances count is zero");

                VkBufferDeviceAddressInfo addressInfo = {};
                addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
                addressInfo.pNext = nullptr;
                addressInfo.buffer = static_cast<Buffer*>(descriptor->GetInstancesBuffer().get())->GetBufferMemoryView()->GetNativeBuffer();
                tlasInstancesGpuAddress = device.GetContext().GetBufferDeviceAddress(device.GetNativeDevice(), &addressInfo);
                buffers.m_instanceCount = descriptor->GetNumInstancesInBuffer();
            }
            
            buffers.m_geometry = {};
            buffers.m_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
            buffers.m_geometry.pNext = nullptr;
            buffers.m_geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
            buffers.m_geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
            buffers.m_geometry.geometry.instances.arrayOfPointers = VK_FALSE;
            buffers.m_geometry.geometry.instances.data.deviceAddress = tlasInstancesGpuAddress;
            
            buffers.m_buildInfo = {};
            buffers.m_buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
            buffers.m_buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
            buffers.m_buildInfo.geometryCount = 1;
            buffers.m_buildInfo.pGeometries = &buffers.m_geometry;
            buffers.m_buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
            buffers.m_buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
            buffers.m_buildInfo.srcAccelerationStructure = nullptr;
            
            VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo = {};
            buildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

            device.GetContext().GetAccelerationStructureBuildSizesKHR(
                device.GetNativeDevice(),
                VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                &buffers.m_buildInfo,
                &buffers.m_instanceCount,
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
            AZ_Assert(resultCode == RHI::ResultCode::Success, "failed to create TLAS scratch buffer");
            
            BufferMemoryView* scratchMemoryView = static_cast<Buffer*>(buffers.m_scratchBuffer.get())->GetBufferMemoryView();
            scratchMemoryView->SetName("TLAS Scratch");

            // create TLAS buffer
            buffers.m_tlasBuffer = RHI::Factory::Get().CreateBuffer();
            AZ::RHI::BufferDescriptor tlasBufferDescriptor;
            tlasBufferDescriptor.m_bindFlags = RHI::BufferBindFlags::RayTracingAccelerationStructure;
            tlasBufferDescriptor.m_byteCount = buildSizesInfo.accelerationStructureSize;
            
            AZ::RHI::DeviceBufferInitRequest tlasBufferRequest;
            tlasBufferRequest.m_buffer = buffers.m_tlasBuffer.get();
            tlasBufferRequest.m_descriptor = tlasBufferDescriptor;
            resultCode = bufferPools.GetTlasBufferPool()->InitBuffer(tlasBufferRequest);
            AZ_Assert(resultCode == RHI::ResultCode::Success, "failed to create TLAS buffer");
            
            BufferMemoryView* tlasMemoryView = static_cast<Buffer*>(buffers.m_tlasBuffer.get())->GetBufferMemoryView();
            tlasMemoryView->SetName("TLAS");

            // create acceleration structure
            VkAccelerationStructureCreateInfoKHR createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
            createInfo.pNext = nullptr;
            createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
            createInfo.size = buildSizesInfo.accelerationStructureSize;
            createInfo.offset = 0;
            createInfo.buffer = tlasMemoryView->GetNativeBuffer();

            buffers.m_accelerationStructure = RayTracingAccelerationStructure::Create();
            buffers.m_accelerationStructure->Init(device, createInfo);
            buffers.m_accelerationStructure->SetBlasBuffers(std::move(blasBuffers));

            buffers.m_buildInfo.dstAccelerationStructure = buffers.m_accelerationStructure->GetNativeAccelerationStructure();

            VkBufferDeviceAddressInfo addressInfo = {};
            addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
            addressInfo.pNext = nullptr;
            addressInfo.buffer = scratchMemoryView->GetNativeBuffer();
            buffers.m_buildInfo.scratchData.deviceAddress =
                device.GetContext().GetBufferDeviceAddress(device.GetNativeDevice(), &addressInfo);

            buffers.m_offsetInfo = {};
            buffers.m_offsetInfo.primitiveCount = buffers.m_instanceCount;

            // store the VkAccelerationStructureKHR in the TLAS Buffer, this is necessary since we need it to
            // setup the TLAS in the DescriptorSet when the Srg is compiled
            static_cast<Buffer*>(buffers.m_tlasBuffer.get())->SetNativeAccelerationStructure(buffers.m_accelerationStructure);

            return RHI::ResultCode::Success;
        }
    } // namespace Vulkan
}
