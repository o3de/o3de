/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DevicePipelineState.h>
#include <RHI/PipelineLayout.h>
#include <Metal/Metal.h>

namespace AZ
{
    namespace Metal
    {
        class ShaderStageFunction;
        struct RasterizerState
        {
            MTLCullMode         m_cullMode = MTLCullModeNone;
            float               m_depthBias = 0.0f;
            float               m_depthSlopeScale = 0.0f;
            float               m_depthBiasClamp = 0.0f;
            MTLWinding          m_frontFaceWinding = MTLWindingClockwise;
            MTLTriangleFillMode m_triangleFillMode = MTLTriangleFillModeFill;
            MTLDepthClipMode    m_depthClipMode = MTLDepthClipModeClip;
            HashValue64         m_hash = HashValue64{ 0 };
            
            void UpdateHash();
        };

        class PipelineState final
            : public RHI::DevicePipelineState
        {
        public:
            AZ_CLASS_ALLOCATOR(PipelineState, AZ::SystemAllocator);

            static RHI::Ptr<PipelineState> Create();
            
            /// Returns the type of pipeline state (draw / dispatch).
            //PipelineType GetType() const;

            /// Returns the pipeline layout associated with this PSO.
            const PipelineLayout* GetPipelineLayout() const;

            /// Returns the platform pipeline state object.
            id<MTLRenderPipelineState> GetGraphicsPipelineState() const;
            id<MTLComputePipelineState> GetComputePipelineState() const;
            
            /// Returns the platform depth stencil state.
            id<MTLDepthStencilState> GetDepthStencilState() const;

            /// Returns the platform rasterizer state.
            RasterizerState GetRasterizerState() const;

            /// Returns the Pipeline topology
            MTLPrimitiveType GetPipelineTopology() const;

            /// Cache multisample state. Used mainly to validate the state against the ove provided by the MSAA image descriptor
            RHI::MultisampleState m_pipelineStateMultiSampleState;
            
        private:
            PipelineState() = default;
            
            //////////////////////////////////////////////////////////////////////////
            // RHI::DevicePipelineState
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::PipelineStateDescriptorForDraw& descriptor, RHI::DevicePipelineLibrary* pipelineLibrary) override;
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::PipelineStateDescriptorForDispatch& descriptor, RHI::DevicePipelineLibrary* pipelineLibrary) override;
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::PipelineStateDescriptorForRayTracing& descriptor, RHI::DevicePipelineLibrary* pipelineLibrary) override;
            void ShutdownInternal() override;
            //////////////////////////////////////////////////////////////////////////

            id<MTLFunction> CompileShader(id<MTLDevice> mtlDevice, const AZStd::string_view filePath, const AZStd::string_view entryPoint, const ShaderStageFunction* shaderFunction, MTLFunctionConstantValues* constantValues);
            id<MTLFunction> ExtractMtlFunction(id<MTLDevice> mtlDevice, const RHI::ShaderStageFunction* stageFunc, MTLFunctionConstantValues* constantValues);
            MTLFunctionConstantValues* CreateFunctionConstantsValues(const RHI::PipelineStateDescriptor& pipelineDescriptor) const;
            
            RHI::ConstPtr<PipelineLayout> m_pipelineLayout;
            AZStd::atomic_bool m_isCompiled = {false};

            // PSOs + descriptors
            id<MTLRenderPipelineState> m_graphicsPipelineState = nil;
            id<MTLComputePipelineState> m_computePipelineState = nil;
            id<MTLDepthStencilState> m_depthStencilState = nil;
            MTLRenderPipelineDescriptor* m_renderPipelineDesc = nil;
            MTLComputePipelineDescriptor* m_computePipelineDesc = nil;
            
            RasterizerState m_rasterizerState;
            MTLPrimitiveType m_primitiveTopology = MTLPrimitiveTypeTriangle;
            AZStd::array<int, RHI::Limits::Pipeline::AttachmentColorCountMax> m_attachmentIndexToColorIndex;
        };
    }
}
