/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/RayTracingPipelineState.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    namespace Vulkan
    {
        class RayTracingPipelineState final
            : public RHI::RayTracingPipelineState
        {
        public:
            AZ_CLASS_ALLOCATOR(RayTracingPipelineState, AZ::SystemAllocator, 0);

            static RHI::Ptr<RayTracingPipelineState> Create();

            const VkPipeline& GetNativePipeline() const { return m_pipeline; }
            const VkPipelineLayout& GetNativePipelineLayout() const { return m_pipelineLayout; }

            typedef AZStd::map<AZStd::string_view, uint8_t*> ShaderHandleMap;
            uint8_t* GetShaderHandle(const AZ::Name& shaderName) const;

        private:
            RayTracingPipelineState() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::PipelineState
            RHI::ResultCode InitInternal(RHI::Device& deviceBase, const RHI::RayTracingPipelineStateDescriptor* descriptor) override;
            void ShutdownInternal() override;
            //////////////////////////////////////////////////////////////////////////

            VkPipeline m_pipeline;
            VkPipelineLayout m_pipelineLayout;
            AZStd::vector<VkShaderModule> m_shaderModules;
            AZStd::vector<uint8_t> m_shaderHandleData;
            ShaderHandleMap m_shaderHandles;
        };
    }
}
