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
#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/DrawItem.h>
#include <Atom/RHI/ScopeProducer.h>

#include <Atom/RPI.Public/Pass/RenderPass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Shader/ShaderReloadNotificationBus.h>

namespace AZ
{
    namespace RPI
    {
        //! A pass that renders to the entire screen using a single triangle.
        //! This pass is designed to work without any dedicated geometry buffers
        //! and instead issues a 3-vertex draw and relies on the vertex shader
        //! to generate a fullscreen triangle from the vertex ids.
        class FullscreenTrianglePass
            : public RenderPass
            , public ShaderReloadNotificationBus::Handler
        {
            AZ_RPI_PASS(FullscreenTrianglePass);

        public:
            AZ_RTTI(FullscreenTrianglePass, "{58C1EDD7-0459-4128-BB20-9839BA233AED}", RenderPass);
            AZ_CLASS_ALLOCATOR(FullscreenTrianglePass, SystemAllocator, 0);
            virtual ~FullscreenTrianglePass();

            //! Creates a FullscreenTrianglePass
            static Ptr<FullscreenTrianglePass> Create(const PassDescriptor& descriptor);

            // Clears the initialized flag so that the next time PrepareFrameInternal is called, it will update the pipeline state
            void Invalidate();

        protected:
            FullscreenTrianglePass(const PassDescriptor& descriptor);
            virtual void Init();

            // Pass behavior overrides...
            void FrameBeginInternal(FramePrepareParams params) override;

            RHI::Viewport m_viewportState;
            RHI::Scissor m_scissorState;

            // The fullscreen shader that will be used by the pass
            Data::Instance<Shader> m_shader;

            // The draw item submitted by this pass
            RHI::DrawItem m_item;

            // The stencil reference value for the draw item
            uint32_t m_stencilRef;

            RPI::ShaderVariantStableId m_shaderVariantStableId = RPI::ShaderAsset::RootShaderVariantStableId;

            Data::Instance<ShaderResourceGroup> m_drawShaderResourceGroup;

            // Scope producer functions...
            virtual void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph, const PassScopeProducer& producer);
            virtual void CompileResources(const RHI::FrameGraphCompileContext& context, const PassScopeProducer& producer);
            virtual void BuildCommandList(const RHI::FrameGraphExecuteContext& context, const PassScopeProducer& producer);

        private:

            ///////////////////////////////////////////////////////////////////
            // ShaderReloadNotificationBus overrides...
            void OnShaderReinitialized(const Shader& shader) override;
            void OnShaderAssetReinitialized(const Data::Asset<ShaderAsset>& shaderAsset) override;
            void OnShaderVariantReinitialized(const Shader& shader, const ShaderVariantId& shaderVariantId, ShaderVariantStableId shaderVariantStableId) override;
            ///////////////////////////////////////////////////////////////////

            void LoadShader();

            PassDescriptor m_passDescriptor;
        };
    }   // namespace RPI
}   // namespace AZ
