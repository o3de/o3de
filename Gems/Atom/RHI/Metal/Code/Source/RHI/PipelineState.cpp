/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "Atom_RHI_Metal_precompiled.h"

#include <Atom/RHI/PipelineStateDescriptor.h>
#include <Atom/RHI.Reflect/Metal/PipelineLayoutDescriptor.h>
#include <Atom/RHI.Reflect/Metal/ShaderStageFunction.h>
#include <Atom/RHI.Reflect/ShaderStageFunction.h>
#include <AzCore/Debug/EventTrace.h>
#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <RHI/PipelineState.h>

namespace AZ
{
    namespace Metal
    {
        RHI::Ptr<PipelineState> PipelineState::Create()
        {
            return aznew PipelineState;
        }

        const PipelineLayout& PipelineState::GetPipelineLayout() const
        {
            return *m_pipelineLayout;
        }

        id<MTLRenderPipelineState> PipelineState::GetGraphicsPipelineState() const
        {
            return m_graphicsPipelineState;
        }
        
        id<MTLComputePipelineState> PipelineState::GetComputePipelineState() const
        {
            return m_computePipelineState;
        }
        
        id<MTLDepthStencilState> PipelineState::GetDepthStencilState() const
        {
            return m_depthStencilState;
        }

        RasterizerState PipelineState::GetRasterizerState() const
        {
            return m_rasterizerState;
        }

        MTLPrimitiveType PipelineState::GetPipelineTopology() const
        {
            return m_primitiveTopology;
        }
        
        id<MTLFunction> PipelineState::CompileShader(id<MTLDevice> mtlDevice, const AZStd::string_view sourceStr, const AZStd::string_view entryPoint, const ShaderStageFunction* shaderFunction)
        {
            id<MTLFunction> pFunction = nullptr;
            NSString* source = [[NSString alloc] initWithCString : sourceStr.data() encoding: NSASCIIStringEncoding];
            
            NSError* error = nil;
            id<MTLLibrary> lib = nil;

            const uint8_t* shaderByteCode = reinterpret_cast<const uint8_t*>(shaderFunction->GetByteCode().data());
            const int byteCodeLength = shaderFunction->GetByteCode().size();
            if(byteCodeLength > 0 )
            {
                dispatch_data_t dispatchByteCodeData = dispatch_data_create(shaderByteCode, byteCodeLength, NULL, DISPATCH_DATA_DESTRUCTOR_DEFAULT);
                lib = [mtlDevice newLibraryWithData:dispatchByteCodeData error:&error];
            }
            else
            {
                //In case byte code was not generated try to create the lib with source code
                MTLCompileOptions* compileOptions = [MTLCompileOptions alloc];
                compileOptions.fastMathEnabled = YES;
                compileOptions.languageVersion = MTLLanguageVersion2_0;
                lib = [mtlDevice newLibraryWithSource:source
                                               options:compileOptions
                                                 error:&error];
                [compileOptions release];
            }
            
            if (error)
            {
                // Error code 4 is warning, but sometimes a 3 (compile error) is returned on warnings only.
                // The documentation indicates that if the lib is nil there is a compile error, otherwise anything
                // in the error is really a warning. Therefore, we check the lib instead of the error code
                AZ_Assert(lib, "HLSLcc compiler should generate valid code.");
                error = 0;
            }
            
            if (lib)
            {
                NSString* entryPointStr = [[NSString alloc] initWithCString : entryPoint.data() encoding: NSASCIIStringEncoding];
                pFunction = [lib newFunctionWithName:entryPointStr];
                [lib release];
                lib = nil;
            }
            
            AZ_Assert(pFunction, "Shader did not compile");
            
            [source release];
            source = nil;
            
            
            return pFunction;
        }
        
        RHI::ResultCode PipelineState::InitInternal(RHI::Device& deviceBase,
                                                    const RHI::PipelineStateDescriptorForDraw& descriptor,
                                                    RHI::PipelineLibrary* pipelineLibraryBase)
        {
            Device& device = static_cast<Device&>(deviceBase);
            RHI::ConstPtr<PipelineLayout> pipelineLayout = device.AcquirePipelineLayout(*descriptor.m_pipelineLayoutDescriptor);
            AZ_Assert(pipelineLayout, "PipelineLayout can not be null");
            
            const RHI::RenderAttachmentConfiguration& attachmentsConfiguration = descriptor.m_renderAttachmentConfiguration;
            MTLRenderPipelineDescriptor* desc = [[MTLRenderPipelineDescriptor alloc] init];
            
            for (AZ::u32 i = 0; i < attachmentsConfiguration.GetRenderTargetCount(); ++i)
            {
                desc.colorAttachments[i].pixelFormat = ConvertPixelFormat(attachmentsConfiguration.GetRenderTargetFormat(i));
                desc.colorAttachments[i].writeMask = ConvertColorWriteMask(descriptor.m_renderStates.m_blendState.m_targets[i].m_writeMask);
                desc.colorAttachments[i].blendingEnabled = descriptor.m_renderStates.m_blendState.m_targets[i].m_enable;
                desc.colorAttachments[i].alphaBlendOperation = ConvertBlendOp(descriptor.m_renderStates.m_blendState.m_targets[i].m_blendAlphaOp);
                desc.colorAttachments[i].rgbBlendOperation = ConvertBlendOp(descriptor.m_renderStates.m_blendState.m_targets[i].m_blendOp);
                desc.colorAttachments[i].destinationAlphaBlendFactor = ConvertBlendFactor(descriptor.m_renderStates.m_blendState.m_targets[i].m_blendAlphaDest);
                desc.colorAttachments[i].destinationRGBBlendFactor = ConvertBlendFactor(descriptor.m_renderStates.m_blendState.m_targets[i].m_blendDest);;
                desc.colorAttachments[i].sourceAlphaBlendFactor = ConvertBlendFactor(descriptor.m_renderStates.m_blendState.m_targets[i].m_blendAlphaSource);
                desc.colorAttachments[i].sourceRGBBlendFactor = ConvertBlendFactor(descriptor.m_renderStates.m_blendState.m_targets[i].m_blendSource);;
            }
            
            MTLVertexDescriptor* vertexDescriptor = [[MTLVertexDescriptor alloc] init];
            ConvertInputElements(descriptor.m_inputStreamLayout, vertexDescriptor);
            desc.vertexDescriptor = vertexDescriptor;
            [vertexDescriptor release];
            vertexDescriptor = nil;
            
            desc.vertexFunction = ExtractMtlFunction(device.GetMtlDevice(), descriptor.m_vertexFunction.get());
            AZ_Assert(desc.vertexFunction, "Vertex mtlFuntion can not be null");
            desc.fragmentFunction = ExtractMtlFunction(device.GetMtlDevice(), descriptor.m_fragmentFunction.get());
            
            RHI::Format depthStencilFormat = attachmentsConfiguration.GetDepthStencilFormat();
            if(descriptor.m_renderStates.m_depthStencilState.m_stencil.m_enable || IsDepthStencilMerged(depthStencilFormat))
            {
                desc.stencilAttachmentPixelFormat = ConvertPixelFormat(depthStencilFormat);
            }
            
            //Depthstencil state
            if(descriptor.m_renderStates.m_depthStencilState.m_depth.m_enable || IsDepthStencilMerged(depthStencilFormat))
            {
                desc.depthAttachmentPixelFormat = ConvertPixelFormat(depthStencilFormat);
                
                MTLDepthStencilDescriptor* depthStencilDesc = [[MTLDepthStencilDescriptor alloc] init];
                ConvertDepthStencilState(descriptor.m_renderStates.m_depthStencilState, depthStencilDesc);
                m_depthStencilState = [device.GetMtlDevice() newDepthStencilStateWithDescriptor:depthStencilDesc];
                AZ_Assert(m_depthStencilState, "Could not create Depth Stencil state.");
                [m_depthStencilState retain];
                [depthStencilDesc release];
                depthStencilDesc = nil;
            }
                        
            desc.sampleCount = descriptor.m_renderStates.m_multisampleState.m_samples;
            desc.alphaToCoverageEnabled = descriptor.m_renderStates.m_blendState.m_alphaToCoverageEnable;
            
            NSError* error = 0;
            MTLRenderPipelineReflection* ref;
            m_graphicsPipelineState = [device.GetMtlDevice() newRenderPipelineStateWithDescriptor:desc options : MTLPipelineOptionBufferTypeInfo reflection : &ref error : &error];
            
            AZ_Assert(m_graphicsPipelineState, "Could not create Pipeline object!.");
            [m_graphicsPipelineState retain];
            [desc release];
            desc = nil;
            
            m_pipelineStateMultiSampleState = descriptor.m_renderStates.m_multisampleState;
            
            //Cache the rasterizer state
            ConvertRasterState(descriptor.m_renderStates.m_rasterState, m_rasterizerState);

            //Save the primitive topology
            m_primitiveTopology = ConvertPrimitiveTopology(descriptor.m_inputStreamLayout.GetTopology());
            
            if (m_graphicsPipelineState)
            {
                m_pipelineLayout = AZStd::move(pipelineLayout);
                return RHI::ResultCode::Success;
            }
            else
            {
                const char * errorStr = [ error.localizedDescription UTF8String ];
                AZ_Error("PipelineState", false, "Failed to compile compute pipeline state with error: %s.", errorStr);
                return RHI::ResultCode::Fail;
            }
        }

        RHI::ResultCode PipelineState::InitInternal(RHI::Device& deviceBase,
                                                    const RHI::PipelineStateDescriptorForDispatch& descriptor,
                                                    RHI::PipelineLibrary* pipelineLibraryBase)
        {
            Device& device = static_cast<Device&>(deviceBase);
            MTLComputePipelineDescriptor* desc = [[MTLComputePipelineDescriptor alloc] init];
            RHI::ConstPtr<PipelineLayout> pipelineLayout = device.AcquirePipelineLayout(*descriptor.m_pipelineLayoutDescriptor);
            AZ_Assert(pipelineLayout, "PipelineLayout can not be null");
            desc.computeFunction = ExtractMtlFunction(device.GetMtlDevice(), descriptor.m_computeFunction.get());
            AZ_Assert(desc.computeFunction, "Compute mtlFuntion can not be null");
            
            NSError* error = 0;
            MTLComputePipelineReflection* ref;
            m_computePipelineState = [device.GetMtlDevice() newComputePipelineStateWithDescriptor:desc options:MTLPipelineOptionBufferTypeInfo reflection:&ref error:&error];
            
            AZ_Assert(m_computePipelineState, "Could not create Pipeline object!.");
            [m_computePipelineState retain];
            [desc release];
            desc = nil;
            
            if (m_computePipelineState)
            {
                m_pipelineLayout = AZStd::move(pipelineLayout);
                return RHI::ResultCode::Success;
            }
            else
            {
                const char * errorStr = [ error.localizedDescription UTF8String ];
                AZ_Error("PipelineState", false, "Failed to compile compute pipeline state with error: %s.", errorStr);
                return RHI::ResultCode::Fail;
            }
        }

        RHI::ResultCode PipelineState::InitInternal(RHI::Device& device, const RHI::PipelineStateDescriptorForRayTracing& descriptor, RHI::PipelineLibrary* pipelineLibrary)
        {
            // [GFX TODO][ATOM-5268] Implement Metal Ray Tracing
            AZ_Assert(false, "Not implemented");
            return RHI::ResultCode::Fail;
        }

        id<MTLFunction> PipelineState::ExtractMtlFunction(id<MTLDevice> mtlDevice, const RHI::ShaderStageFunction* stageFunc)
        {
            // set the bound shader state settings
            if (stageFunc)
            {
                const ShaderStageFunction* shaderFunction = azrtti_cast<const ShaderStageFunction*>(stageFunc);
                AZStd::string_view strView(shaderFunction->GetSourceCode());
                
                id<MTLFunction> mtlFunction = nil;
                mtlFunction = CompileShader(mtlDevice, strView, shaderFunction->GetEntryFunctionName(), shaderFunction);

                return mtlFunction;
            }
            
            return nil;
        }
        void PipelineState::ShutdownInternal()
        {
            if (m_graphicsPipelineState)
            {
                [m_graphicsPipelineState release];
                m_graphicsPipelineState = nil;
            }
            
            if (m_computePipelineState)
            {
                [m_computePipelineState release];
                m_computePipelineState = nil;
            }
            
            if (m_depthStencilState)
            {
                [m_depthStencilState release];
                m_depthStencilState = nil;
            }
        }
    }
}
