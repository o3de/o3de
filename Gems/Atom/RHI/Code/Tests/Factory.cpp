/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include <Tests/Factory.h>
#include <Atom/RHI/Fence.h>
#include <Atom/RHI/RayTracingAccelerationStructure.h>
#include <Atom/RHI/RayTracingPipelineState.h>
#include <Atom/RHI/RayTracingShaderTable.h>
#include <Atom/RHI/StreamingImagePool.h>
#include <Atom/RHI/SwapChain.h>
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

    RHI::PhysicalDeviceList Factory::EnumeratePhysicalDevices()
    {
        return PhysicalDevice::Enumerate();
    }

    RHI::Ptr<RHI::Device> Factory::CreateDevice()
    {
        return aznew Device;
    }

    RHI::Ptr<RHI::SwapChain> Factory::CreateSwapChain()
    {
        return nullptr;
    }

    RHI::Ptr<RHI::Fence> Factory::CreateFence()
    {
        return nullptr;
    }

    RHI::Ptr<RHI::Buffer> Factory::CreateBuffer()
    {
        return aznew Buffer;
    }

    RHI::Ptr<RHI::BufferView> Factory::CreateBufferView()
    {
        return aznew BufferView;
    }

    RHI::Ptr<RHI::BufferPool> Factory::CreateBufferPool()
    {
        return aznew BufferPool;
    }

    RHI::Ptr<RHI::Image> Factory::CreateImage()
    {
        return aznew Image;
    }

    RHI::Ptr<RHI::ImageView> Factory::CreateImageView()
    {
        return aznew ImageView;
    }

    RHI::Ptr<RHI::ImagePool> Factory::CreateImagePool()
    {
        return aznew ImagePool;
    }

    RHI::Ptr<RHI::StreamingImagePool> Factory::CreateStreamingImagePool()
    {
        return nullptr;
    }

    RHI::Ptr<RHI::ShaderResourceGroupPool> Factory::CreateShaderResourceGroupPool()
    {
        return aznew ShaderResourceGroupPool;
    }

    RHI::Ptr<RHI::ShaderResourceGroup> Factory::CreateShaderResourceGroup()
    {
        return aznew ShaderResourceGroup;
    }

    RHI::Ptr<RHI::PipelineLibrary> Factory::CreatePipelineLibrary()
    {
        return aznew PipelineLibrary;
    }

    RHI::Ptr<RHI::PipelineState> Factory::CreatePipelineState()
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

    RHI::Ptr<RHI::TransientAttachmentPool> Factory::CreateTransientAttachmentPool()
    {
        return aznew TransientAttachmentPool;
    }

    AZ::RHI::Ptr<AZ::RHI::QueryPool> Factory::CreateQueryPool()
    {
        return aznew QueryPool;
    }

    AZ::RHI::Ptr<AZ::RHI::Query> Factory::CreateQuery()
    {
        return aznew Query;
    }

    AZ::RHI::Ptr<AZ::RHI::IndirectBufferSignature> Factory::CreateIndirectBufferSignature()
    {
        return aznew IndirectBufferSignature;
    }

    AZ::RHI::Ptr<AZ::RHI::IndirectBufferWriter> Factory::CreateIndirectBufferWriter()
    {
        return aznew IndirectBufferWriter;
    }

    AZ::RHI::Ptr<AZ::RHI::RayTracingBlas> Factory::CreateRayTracingBlas()
    {
        AZ_Assert(false, "Not implemented");
        return nullptr;
    }

    AZ::RHI::Ptr<AZ::RHI::RayTracingTlas> Factory::CreateRayTracingTlas()
    {
        AZ_Assert(false, "Not implemented");
        return nullptr;
    }

    AZ::RHI::Ptr<AZ::RHI::RayTracingPipelineState> Factory::CreateRayTracingPipelineState()
    {
        AZ_Assert(false, "Not implemented");
        return nullptr;
    }

    AZ::RHI::Ptr<AZ::RHI::RayTracingShaderTable> Factory::CreateRayTracingShaderTable()
    {
        AZ_Assert(false, "Not implemented");
        return nullptr;
    }
}
