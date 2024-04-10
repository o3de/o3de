/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/PipelineLayoutDescriptor.h>
#include <RHI/DescriptorPool.h>
#include <RHI/DescriptorSetLayout.h>
#include <RHI/Device.h>
#include <RHI/GraphicsPipeline.h>
#include <RHI/ComputePipeline.h>
#include <RHI/RayTracingPipeline.h>
#include <RHI/PipelineLayout.h>
#include <RHI/PipelineLibrary.h>
#include <RHI/PipelineState.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<PipelineState> PipelineState::Create()
        {
            return aznew PipelineState();
        }

        PipelineLayout* PipelineState::GetPipelineLayout() const
        {
            if (!m_pipeline)
            {
                return nullptr;
            }
            return m_pipeline->GetPipelineLayout();
        }

        Pipeline* PipelineState::GetPipeline() const
        {
            return m_pipeline.get();
        }

        PipelineLibrary* PipelineState::GetPipelineLibrary() const
        {
            if (!m_pipeline)
            {
                return nullptr;
            }
            return m_pipeline->GetPipelineLibrary();
        }

        RHI::ResultCode PipelineState::InitInternal(RHI::Device& device, const RHI::PipelineStateDescriptorForDraw& descriptor, RHI::PipelineLibrary* pipelineLibrary)
        {
            Pipeline::Descriptor pipelineDescriptor;
            pipelineDescriptor.m_pipelineDescritor = &descriptor;
            pipelineDescriptor.m_name = descriptor.GetName();
            pipelineDescriptor.m_device = static_cast<Device*>(&device);
            pipelineDescriptor.m_pipelineLibrary = static_cast<PipelineLibrary*>(pipelineLibrary);
            RHI::Ptr<GraphicsPipeline> pipeline = GraphicsPipeline::Create();
            RHI::ResultCode result = pipeline->Init(pipelineDescriptor);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            m_pipeline = pipeline;            
            return result;
        }

        RHI::ResultCode PipelineState::InitInternal(RHI::Device& device, const RHI::PipelineStateDescriptorForDispatch& descriptor, RHI::PipelineLibrary* pipelineLibrary)
        {
            Pipeline::Descriptor pipelineDescriptor;
            pipelineDescriptor.m_pipelineDescritor = &descriptor;
            pipelineDescriptor.m_name = descriptor.GetName();
            pipelineDescriptor.m_device = static_cast<Device*>(&device);
            pipelineDescriptor.m_pipelineLibrary = static_cast<PipelineLibrary*>(pipelineLibrary);
            RHI::Ptr<ComputePipeline> pipeline = ComputePipeline::Create();
            RHI::ResultCode result = pipeline->Init(pipelineDescriptor);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            m_pipeline = pipeline;
            return result;
        }

        RHI::ResultCode PipelineState::InitInternal(RHI::Device& device, const RHI::PipelineStateDescriptorForRayTracing& descriptor, RHI::PipelineLibrary* pipelineLibrary)
        {
            Pipeline::Descriptor pipelineDescriptor;
            pipelineDescriptor.m_pipelineDescritor = &descriptor;
            pipelineDescriptor.m_name = descriptor.GetName();
            pipelineDescriptor.m_device = static_cast<Device*>(&device);
            pipelineDescriptor.m_pipelineLibrary = static_cast<PipelineLibrary*>(pipelineLibrary);
            RHI::Ptr<RayTracingPipeline> pipeline = RayTracingPipeline::Create();
            RHI::ResultCode result = pipeline->Init(pipelineDescriptor);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            m_pipeline = pipeline;
            return result;
        }

        void PipelineState::ShutdownInternal()
        {
            auto& device = static_cast<Device&>(GetDevice());
            if (m_pipeline)
            {
                device.QueueForRelease(m_pipeline);
                m_pipeline = nullptr;
            }
        }

        void PipelineState::SetNameInternal(const AZStd::string_view& name)
        {
            if (m_pipeline)
            {
                m_pipeline->SetName(AZ::Name(name));
            }
        }
    }
}
