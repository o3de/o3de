/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DevicePipelineState.h>
#include <RHI/Pipeline.h>
#include <RHI/PipelineLayout.h>

namespace AZ
{
    namespace Vulkan
    {
        class PipelineState final
            : public RHI::DevicePipelineState 
        {
            using Base = RHI::DevicePipelineState;

        public:
            AZ_CLASS_ALLOCATOR(PipelineState, AZ::SystemAllocator);
            AZ_RTTI(PipelineState, "40D76567-1C43-4D0F-885A-B63337DCE31B", Base);

            static RHI::Ptr<PipelineState> Create();

            PipelineLayout* GetPipelineLayout() const;
            Pipeline* GetPipeline() const;
            PipelineLibrary* GetPipelineLibrary() const;

        private:
            //////////////////////////////////////////////////////////////////////////
            // RHI::DevicePipelineState
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::PipelineStateDescriptorForDraw& descriptor, RHI::DevicePipelineLibrary* pipelineLibrary) override;
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::PipelineStateDescriptorForDispatch& descriptor, RHI::DevicePipelineLibrary* pipelineLibrary) override;
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::PipelineStateDescriptorForRayTracing& descriptor, RHI::DevicePipelineLibrary* pipelineLibrary) override;
            void ShutdownInternal() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////

            RHI::Ptr<Pipeline> m_pipeline;
        };
    }
}
