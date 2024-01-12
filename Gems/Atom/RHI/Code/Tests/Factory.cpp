/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Tests/Factory.h>
#include <Atom/RHI/DispatchRaysIndirectBuffer.h>
#include <Atom/RHI/SingleDeviceFence.h>
#include <Atom/RHI/SingleDeviceRayTracingAccelerationStructure.h>
#include <Atom/RHI/SingleDeviceRayTracingPipelineState.h>
#include <Atom/RHI/SingleDeviceRayTracingShaderTable.h>
#include <Atom/RHI/SingleDeviceRayTracingBufferPools.h>
#include <Atom/RHI/SingleDeviceStreamingImagePool.h>
#include <Atom/RHI/SingleDeviceSwapChain.h>
#include <Tests/Device.h>
#include <Tests/ShaderResourceGroup.h>
#include <Tests/PipelineState.h>
#include <Tests/Query.h>
#include <Tests/IndirectBuffer.h>

namespace UnitTest
{
    using namespace AZ;

    Factory::Factory()
    {
        RHI::Factory::Register(this);
    }

    Factory::~Factory()
    {
        RHI::Factory::Unregister(this);

        RHI::ResourceInvalidateBus::AllowFunctionQueuing(false);
        RHI::ResourceInvalidateBus::ClearQueuedEvents();
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

    RHI::Ptr<RHI::SingleDeviceSwapChain> Factory::CreateSwapChain()
    {
        return nullptr;
    }

    RHI::Ptr<RHI::SingleDeviceFence> Factory::CreateFence()
    {
        return nullptr;
    }

    RHI::Ptr<RHI::SingleDeviceBuffer> Factory::CreateBuffer()
    {
        return aznew Buffer;
    }

    RHI::Ptr<RHI::SingleDeviceBufferView> Factory::CreateBufferView()
    {
        return aznew BufferView;
    }

    RHI::Ptr<RHI::SingleDeviceBufferPool> Factory::CreateBufferPool()
    {
        return aznew BufferPool;
    }

    RHI::Ptr<RHI::SingleDeviceImage> Factory::CreateImage()
    {
        return aznew Image;
    }

    RHI::Ptr<RHI::SingleDeviceImageView> Factory::CreateImageView()
    {
        return aznew ImageView;
    }

    RHI::Ptr<RHI::SingleDeviceImagePool> Factory::CreateImagePool()
    {
        return aznew ImagePool;
    }

    RHI::Ptr<RHI::SingleDeviceStreamingImagePool> Factory::CreateStreamingImagePool()
    {
        return nullptr;
    }

    RHI::Ptr<RHI::SingleDeviceShaderResourceGroupPool> Factory::CreateShaderResourceGroupPool()
    {
        return aznew ShaderResourceGroupPool;
    }

    RHI::Ptr<RHI::SingleDeviceShaderResourceGroup> Factory::CreateShaderResourceGroup()
    {
        return aznew ShaderResourceGroup;
    }

    RHI::Ptr<RHI::SingleDevicePipelineLibrary> Factory::CreatePipelineLibrary()
    {
        return aznew PipelineLibrary;
    }

    RHI::Ptr<RHI::SingleDevicePipelineState> Factory::CreatePipelineState()
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

    RHI::Ptr<RHI::SingleDeviceTransientAttachmentPool> Factory::CreateTransientAttachmentPool()
    {
        return aznew TransientAttachmentPool;
    }

    AZ::RHI::Ptr<AZ::RHI::SingleDeviceQueryPool> Factory::CreateQueryPool()
    {
        return aznew QueryPool;
    }

    AZ::RHI::Ptr<AZ::RHI::SingleDeviceQuery> Factory::CreateQuery()
    {
        return aznew Query;
    }

    AZ::RHI::Ptr<AZ::RHI::SingleDeviceIndirectBufferSignature> Factory::CreateIndirectBufferSignature()
    {
        return aznew NiceIndirectBufferSignature;
    }

    AZ::RHI::Ptr<AZ::RHI::SingleDeviceIndirectBufferWriter> Factory::CreateIndirectBufferWriter()
    {
        return aznew NiceIndirectBufferWriter;
    }

    AZ::RHI::Ptr<AZ::RHI::SingleDeviceRayTracingBufferPools> Factory::CreateRayTracingBufferPools()
    {
        AZ_Assert(false, "Not implemented");
        return nullptr;
    }

    AZ::RHI::Ptr<AZ::RHI::SingleDeviceRayTracingBlas> Factory::CreateRayTracingBlas()
    {
        AZ_Assert(false, "Not implemented");
        return nullptr;
    }

    AZ::RHI::Ptr<AZ::RHI::SingleDeviceRayTracingTlas> Factory::CreateRayTracingTlas()
    {
        AZ_Assert(false, "Not implemented");
        return nullptr;
    }

    AZ::RHI::Ptr<AZ::RHI::SingleDeviceRayTracingPipelineState> Factory::CreateRayTracingPipelineState()
    {
        AZ_Assert(false, "Not implemented");
        return nullptr;
    }

    AZ::RHI::Ptr<AZ::RHI::SingleDeviceRayTracingShaderTable> Factory::CreateRayTracingShaderTable()
    {
        AZ_Assert(false, "Not implemented");
        return nullptr;
    }

    RHI::Ptr<RHI::DispatchRaysIndirectBuffer> Factory::CreateDispatchRaysIndirectBuffer()
    {
        AZ_Assert(false, "Not implemented");
        return nullptr;
    }
}
