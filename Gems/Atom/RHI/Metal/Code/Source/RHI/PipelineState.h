/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/SingleDevicePipelineState.h>
#include <RHI/PipelineLayout.h>
#include <Metal/Metal.h>

namespace AZ
{
    namespace Metal
    {
        class ShaderStageFunction;
        struct RasterizerState
        {
            MTLCullMode         m_cullMode;
            float               m_depthBias;
            float               m_depthSlopeScale;
            float               m_depthBiasClamp;
            MTLWinding          m_frontFaceWinding;
            MTLTriangleFillMode m_triangleFillMode;
            MTLDepthClipMode    m_depthClipMode;
        };

        class PipelineState final
            : public RHI::SingleDevicePipelineState
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
            // RHI::SingleDevicePipelineState
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::PipelineStateDescriptorForDraw& descriptor, RHI::SingleDevicePipelineLibrary* pipelineLibrary) override;
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::PipelineStateDescriptorForDispatch& descriptor, RHI::SingleDevicePipelineLibrary* pipelineLibrary) override;
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::PipelineStateDescriptorForRayTracing& descriptor, RHI::SingleDevicePipelineLibrary* pipelineLibrary) override;
            void ShutdownInternal() override;
            //////////////////////////////////////////////////////////////////////////

            id<MTLFunction> CompileShader(id<MTLDevice> mtlDevice, const AZStd::string_view filePath, const AZStd::string_view entryPoint, const ShaderStageFunction* shaderFunction);
            id<MTLFunction> ExtractMtlFunction(id<MTLDevice> mtlDevice, const RHI::ShaderStageFunction* stageFunc);
            
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
        };
    }
}
