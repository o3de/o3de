/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RHI/RayTracingPipelineState.h>
#include <Atom/RHI/RayTracingShaderTable.h>
#include <Atom/RPI.Public/Pass/RenderPass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderReloadNotificationBus.h>

namespace AZ
{
    namespace Render
    {
        struct RayTracingPassData;

        //! This pass executes a raytracing shader as specified in the PassData.
        class RayTracingPass
            : public RPI::RenderPass
            , private RPI::ShaderReloadNotificationBus::MultiHandler
        {
            AZ_RPI_PASS(RayTracingPass);

        public:
            AZ_RTTI(RayTracingPass, "{7A68A36E-956A-4258-93FE-38686042C4D9}", RPI::RenderPass);
            AZ_CLASS_ALLOCATOR(RayTracingPass, SystemAllocator, 0);
            virtual ~RayTracingPass();

            //! Creates a RayTracingPass
            static RPI::Ptr<RayTracingPass> Create(const RPI::PassDescriptor& descriptor);

        protected:
            RayTracingPass(const RPI::PassDescriptor& descriptor);

            // Pass overrides
            void FrameBeginInternal(FramePrepareParams params) override;

            // Scope producer functions
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

            // ShaderReloadNotificationBus::Handler overrides
            void OnShaderReinitialized(const RPI::Shader& shader) override;
            void OnShaderAssetReinitialized(const Data::Asset<RPI::ShaderAsset>& shaderAsset) override;
            void OnShaderVariantReinitialized(const RPI::ShaderVariant& shaderVariant) override;

            // load the raytracing shaders and setup pipeline states
            void Init();

            // helper for loading a shader from a shader asset reference
            Data::Instance<RPI::Shader> LoadShader(const RPI::AssetReference& shaderAssetReference);

            // pass data
            RPI::PassDescriptor m_passDescriptor;
            const RayTracingPassData* m_passData = nullptr;

            // revision number of the ray tracing TLAS when the shader table was built
            uint32_t m_rayTracingRevision = 0;

            // raytracing shaders, pipeline states, and shader table
            Data::Instance<RPI::Shader> m_rayGenerationShader;
            Data::Instance<RPI::Shader> m_missShader;
            Data::Instance<RPI::Shader> m_closestHitShader;
            RHI::Ptr<RHI::RayTracingPipelineState> m_rayTracingPipelineState;
            RHI::ConstPtr<RHI::PipelineState> m_globalPipelineState;
            RHI::Ptr<RHI::RayTracingShaderTable> m_rayTracingShaderTable;
            bool m_requiresViewSrg = false;
            bool m_requiresSceneSrg = false;
            bool m_requiresRayTracingMaterialSrg = false;
        };
    }   // namespace RPI
}   // namespace AZ
