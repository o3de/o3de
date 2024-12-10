/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/DeviceDrawItem.h>
#include <Atom/RHI/ScopeProducer.h>

#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Public/Pass/RenderPass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Shader/ShaderReloadNotificationBus.h>
#include <Atom/RPI.Public/PipelineState.h>

namespace AZ
{
    namespace RPI
    {
        //! A pass that renders to the entire screen using a single triangle.
        //! This pass is designed to work without any dedicated geometry buffers
        //! and instead issues a 3-vertex draw and relies on the vertex shader
        //! to generate a fullscreen triangle from the vertex ids.
        class ATOM_RPI_PUBLIC_API FullscreenTrianglePass
            : public RenderPass
            , public ShaderReloadNotificationBus::Handler
        {
            AZ_RPI_PASS(FullscreenTrianglePass);

        public:
            AZ_RTTI(FullscreenTrianglePass, "{58C1EDD7-0459-4128-BB20-9839BA233AED}", RenderPass);
            AZ_CLASS_ALLOCATOR(FullscreenTrianglePass, SystemAllocator);
            virtual ~FullscreenTrianglePass();

            //! Creates a FullscreenTrianglePass
            static Ptr<FullscreenTrianglePass> Create(const PassDescriptor& descriptor);

            //! Return the shader
            Data::Instance<Shader> GetShader() const;

            //! Updates the shader options used in this pass
            void UpdateShaderOptions(const ShaderOptionList& shaderOptions);
            void UpdateShaderOptions(const ShaderVariantId& shaderVariantId);

        protected:
            FullscreenTrianglePass(const PassDescriptor& descriptor);

            // Pass behavior overrides...
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

            // Scope producer functions...
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

            // ShaderReloadNotificationBus overrides...
            void OnShaderReinitialized(const Shader& shader) override;
            void OnShaderAssetReinitialized(const Data::Asset<ShaderAsset>& shaderAsset) override;
            void OnShaderVariantReinitialized(const ShaderVariant& shaderVariant) override;

            // Common code when updating the shader variant with new options
            void UpdateShaderOptionsCommon();

            RHI::Viewport m_viewportState;
            RHI::Scissor m_scissorState;

            // The fullscreen shader that will be used by the pass
            Data::Instance<Shader> m_shader;

            // Manages changes to the pipeline state
            PipelineStateForDraw m_pipelineStateForDraw;

            // The draw item submitted by this pass
            RHI::DrawItem m_item;

            // Holds the geometry info for the draw call
            RHI::GeometryView m_geometryView;

            // The stencil reference value for the draw item
            uint32_t m_stencilRef;

            Data::Instance<ShaderResourceGroup> m_drawShaderResourceGroup;

        private:
            void LoadShader();
            void UpdateSrgs();
            void BuildDrawItem();

            PassDescriptor m_passDescriptor;
        };
    }   // namespace RPI
}   // namespace AZ
