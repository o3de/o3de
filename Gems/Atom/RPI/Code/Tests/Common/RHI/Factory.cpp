/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/DeviceDispatchRaysIndirectBuffer.h>
#include <Atom/RHI/DeviceFence.h>
#include <Atom/RHI/DevicePipelineState.h>
#include <Atom/RHI/DeviceRayTracingAccelerationStructure.h>
#include <Atom/RHI/DeviceRayTracingBufferPools.h>
#include <Atom/RHI/DeviceRayTracingCompactionQueryPool.h>
#include <Atom/RHI/DeviceRayTracingPipelineState.h>
#include <Atom/RHI/DeviceRayTracingShaderTable.h>
#include <Atom/RHI/DeviceShaderResourceGroupPool.h>
#include <Atom/RHI/DeviceSwapChain.h>
#include <Atom/RHI/DeviceTransientAttachmentPool.h>
#include <Atom/RHI/FrameGraphCompiler.h>
#include <Atom/RHI/FrameGraphExecuter.h>
#include <Common/RHI/Factory.h>
#include <Common/RHI/Stubs.h>


namespace UnitTest
{
    namespace StubRHI
    {
        using namespace AZ;
        
        Factory::Factory() :
            m_platformName{"UnitTest"}
        {
            RHI::Factory::Register(this);
        }

        Factory::~Factory()
        {
            RHI::Factory::Unregister(this);

            RHI::ResourceInvalidateBus::AllowFunctionQueuing(false);
            RHI::ResourceInvalidateBus::ClearQueuedEvents();
        }

        RHI::Ptr<RHI::Device> Factory::CreateDefaultDevice()
        {
            RHI::PhysicalDeviceList physicalDevices = Get().EnumeratePhysicalDevices();
            AZ_Assert(physicalDevices.size() == 1, "Expected a single physical device.");

            RHI::Ptr<RHI::Device> device = Get().CreateDevice();
            device->Init(RHI::MultiDevice::DefaultDeviceIndex, *physicalDevices[0]);

            return device;
        }

        Name Factory::GetName()
        {
            return m_platformName;
        }

        RHI::APIPriority Factory::GetDefaultPriority()
        {
            return RHI::APIMiddlePriority;
        }

        RHI::APIType Factory::GetType()
        {
            return RHI::APIType(m_platformName.GetStringView());
        }

        bool Factory::SupportsXR() const
        {
            return false;
        }

        RHI::PhysicalDeviceList Factory::EnumeratePhysicalDevices()
        {
            return PhysicalDevice::Enumerate();
        }

        RHI::Ptr<RHI::Device> Factory::CreateDevice()
        {
            return aznew Device;
        }

        RHI::Ptr<RHI::DeviceSwapChain> Factory::CreateSwapChain()
        {
            return aznew SwapChain;
        }

        RHI::Ptr<RHI::DeviceFence> Factory::CreateFence()
        {
            return aznew Fence;
        }

        RHI::Ptr<RHI::DeviceBuffer> Factory::CreateBuffer()
        {
            return aznew Buffer;
        }

        RHI::Ptr<RHI::DeviceBufferView> Factory::CreateBufferView()
        {
            return aznew BufferView;
        }

        RHI::Ptr<RHI::DeviceBufferPool> Factory::CreateBufferPool()
        {
            return aznew BufferPool;
        }

        RHI::Ptr<RHI::DeviceImage> Factory::CreateImage()
        {
            return aznew Image;
        }

        RHI::Ptr<RHI::DeviceImageView> Factory::CreateImageView()
        {
            return aznew ImageView;
        }

        RHI::Ptr<RHI::DeviceImagePool> Factory::CreateImagePool()
        {
            return aznew ImagePool;
        }

        RHI::Ptr<RHI::DeviceStreamingImagePool> Factory::CreateStreamingImagePool()
        {
            return aznew StreamingImagePool;
        }

        RHI::Ptr<RHI::DeviceShaderResourceGroupPool> Factory::CreateShaderResourceGroupPool()
        {
            return aznew ShaderResourceGroupPool;
        }

        RHI::Ptr<RHI::DeviceShaderResourceGroup> Factory::CreateShaderResourceGroup()
        {
            return aznew ShaderResourceGroup;
        }

        RHI::Ptr<RHI::DevicePipelineLibrary> Factory::CreatePipelineLibrary()
        {
            return aznew PipelineLibrary;
        }

        RHI::Ptr<RHI::DevicePipelineState> Factory::CreatePipelineState()
        {
            return aznew PipelineState;
        }

        RHI::Ptr<RHI::Scope> Factory::CreateScope()
        {
            return aznew Scope;
        }

        RHI::Ptr<RHI::FrameGraphCompiler> Factory::CreateFrameGraphCompiler()
        {
            return aznew FrameGraphCompiler;
        }

        RHI::Ptr<RHI::FrameGraphExecuter> Factory::CreateFrameGraphExecuter()
        {
            return aznew FrameGraphExecuter;
        }

        RHI::Ptr<RHI::DeviceTransientAttachmentPool> Factory::CreateTransientAttachmentPool()
        {
            return aznew TransientAttachmentPool;
        }

        AZ::RHI::Ptr<AZ::RHI::DeviceQueryPool> Factory::CreateQueryPool()
        {
            return aznew QueryPool;
        }

        AZ::RHI::Ptr<AZ::RHI::DeviceQuery> Factory::CreateQuery()
        {
            return aznew Query;
        }

        AZ::RHI::Ptr<AZ::RHI::DeviceIndirectBufferSignature> Factory::CreateIndirectBufferSignature()
        {
            return aznew IndirectBufferSignature;
        }

        AZ::RHI::Ptr<AZ::RHI::DeviceIndirectBufferWriter> Factory::CreateIndirectBufferWriter()
        {
            return aznew IndirectBufferWriter;
        }

        AZ::RHI::Ptr<AZ::RHI::DeviceRayTracingBufferPools> Factory::CreateRayTracingBufferPools()
        {
            AZ_Assert(false, "Not implemented");
            return nullptr;
        }

        AZ::RHI::Ptr<AZ::RHI::DeviceRayTracingBlas> Factory::CreateRayTracingBlas()
        {
            AZ_Assert(false, "Not implemented");
            return nullptr;
        }

        AZ::RHI::Ptr<AZ::RHI::DeviceRayTracingTlas> Factory::CreateRayTracingTlas()
        {
            AZ_Assert(false, "Not implemented");
            return nullptr;
        }

        AZ::RHI::Ptr<AZ::RHI::DeviceRayTracingPipelineState> Factory::CreateRayTracingPipelineState()
        {
            AZ_Assert(false, "Not implemented");
            return nullptr;
        }

        AZ::RHI::Ptr<AZ::RHI::DeviceRayTracingShaderTable> Factory::CreateRayTracingShaderTable()
        {
            AZ_Assert(false, "Not implemented");
            return nullptr;
        }

        RHI::Ptr<RHI::DeviceDispatchRaysIndirectBuffer> Factory::CreateDispatchRaysIndirectBuffer()
        {
            AZ_Assert(false, "Not implemented");
            return nullptr;
        }

        AZ::RHI::Ptr<AZ::RHI::DeviceRayTracingCompactionQueryPool> Factory::CreateRayTracingCompactionQueryPool()
        {
            AZ_Assert(false, "Not implemented");
            return nullptr;
        }

        AZ::RHI::Ptr<AZ::RHI::DeviceRayTracingCompactionQuery> Factory::CreateRayTracingCompactionQuery()
        {
            AZ_Assert(false, "Not implemented");
            return nullptr;
        }
    }
}
