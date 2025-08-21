/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DispatchRaysIndirectBuffer.h>
#include <Atom/RHI/DispatchRaysItem.h>
#include <Atom/RHI/RayTracingPipelineState.h>
#include <Atom/RHI/RayTracingShaderTable.h>
#include <Atom/RPI.Public/Pass/RenderPass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderReloadNotificationBus.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    namespace Render
    {
        struct RayTracingMaterialPassData;

        //! This pass executes a raytracing shader as specified in the PassData.
        class RayTracingMaterialPass : public RPI::RenderPass
        {
            AZ_RPI_PASS(RayTracingMaterialPass);

        public:
            AZ_RTTI(RayTracingMaterialPass, "{3E525184-CBA6-447C-9097-7A9872F5B478}", RPI::RenderPass);
            AZ_CLASS_ALLOCATOR(RayTracingMaterialPass, SystemAllocator);
            virtual ~RayTracingMaterialPass();

            //! Creates a RayTracingMaterialPass
            static RPI::Ptr<RayTracingMaterialPass> Create(const RPI::PassDescriptor& descriptor);

            void SetMaxRayLength(float maxRayLength)
            {
                m_maxRayLength = maxRayLength;
            }

            void SetDrawListTag(Name drawListName);

        protected:
            RayTracingMaterialPass(const RPI::PassDescriptor& descriptor);

            // Pass overrides
            bool IsEnabled() const override;
            void BuildInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;
            void FrameEndInternal() override;

            // Scope producer functions
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

            // load the raytracing shaders and setup pipeline states
            void UpdateShaderLibraries();

            // pass data
            RPI::PassDescriptor m_passDescriptor;
            const RayTracingMaterialPassData* m_passData = nullptr;

            RHI::DrawListTag m_drawListTag;

            Name m_fullscreenSizeSourceSlotName;
            bool m_fullscreenDispatch = false;
            RPI::PassAttachmentBinding* m_fullscreenSizeSourceBinding = nullptr;

            bool m_indirectDispatch = false;
            Name m_indirectDispatchBufferSlotName;
            RPI::PassAttachmentBinding* m_indirectDispatchRaysBufferBinding = nullptr;
            RHI::Ptr<RHI::IndirectBufferSignature> m_indirectDispatchRaysBufferSignature;
            RHI::IndirectBufferView m_indirectDispatchRaysBufferView;
            RHI::Ptr<RHI::DispatchRaysIndirectBuffer> m_dispatchRaysIndirectBuffer;

            // revision number of the ray tracing TLAS when the shader table was built
            uint32_t m_rayTracingShaderTableRevision{ std::numeric_limits<uint32_t>::max() };
            uint32_t m_dispatchRaysShaderTableRevision{ std::numeric_limits<uint32_t>::max() };
            uint32_t m_proceduralGeometryTypeRevision = 0;

            // raytracing shaders, pipeline states, and shader table
            RHI::Ptr<RHI::RayTracingPipelineState> m_rayTracingPipelineState;
            RHI::ConstPtr<RHI::PipelineState> m_globalPipelineState;
            RHI::Ptr<RHI::RayTracingShaderTable> m_rayTracingShaderTable;

            // [GFX TODO][ATOM-15610] Add RenderPass::SetSrgsForRayTracingDispatch
            // Remove this as soon as we can use the RenderPass::BindSrg() for raytracing
            AZStd::vector<RHI::ShaderResourceGroup*> m_rayTracingSRGsToBind;

            bool m_requiresViewSrg = false;
            bool m_requiresSceneSrg = false;
            bool m_requiresMaterialSrg = false;
            bool m_requiresRayTracingSceneSrg = false;
            float m_maxRayLength = 1e27f;

            RHI::ShaderInputNameIndex m_maxRayLengthInputIndex = "m_maxRayLength";

            RHI::DispatchRaysItem m_dispatchRaysItem;
        };
    } // namespace Render
} // namespace AZ
