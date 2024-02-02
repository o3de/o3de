/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/PipelineStateDescriptor.h>
#include <Atom/RHI.Reflect/Metal/PipelineLayoutDescriptor.h>
#include <Atom/RHI.Reflect/Metal/ShaderStageFunction.h>
#include <Atom/RHI.Reflect/ShaderStageFunction.h>
#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <RHI/PipelineState.h>
#include <RHI/PipelineLibrary.h>
#include <AzCore/std/algorithm.h>

namespace AZ
{
    namespace Metal
    {
        RHI::Ptr<PipelineState> PipelineState::Create()
        {
            return aznew PipelineState;
        }

        const PipelineLayout* PipelineState::GetPipelineLayout() const
        {
            return m_pipelineLayout.get();
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
            
            bool loadFromByteCode = false;
            
            // MacOS Big Sur (11.16.x) has issue loading some shader's byte code when GPUCapture(Metal) is on.
            // Only enable it for Monterey (12.x)
            if(@available(iOS 14.0, macOS 12.0, *))
            {
                loadFromByteCode = true;
            }
            
            const uint8_t* shaderByteCode = reinterpret_cast<const uint8_t*>(shaderFunction->GetByteCode().data());
            const int byteCodeLength = static_cast<int>(shaderFunction->GetByteCode().size());
            if(byteCodeLength > 0 && loadFromByteCode)
            {
                dispatch_data_t dispatchByteCodeData = dispatch_data_create(shaderByteCode, byteCodeLength, NULL, DISPATCH_DATA_DESTRUCTOR_DEFAULT);
                lib = [mtlDevice newLibraryWithData:dispatchByteCodeData error:&error];
            }
            else
            {
                //In case byte code was not generated try to create the lib with source code
                MTLCompileOptions* compileOptions = [MTLCompileOptions alloc];
                compileOptions.fastMathEnabled = YES;
                compileOptions.languageVersion = MTLLanguageVersion2_2;
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
                                                    RHI::SingleDevicePipelineLibrary* pipelineLibraryBase)
        {
            NSError* error = 0;
            Device& device = static_cast<Device&>(deviceBase);
            RHI::ConstPtr<PipelineLayout> pipelineLayout = device.AcquirePipelineLayout(*descriptor.m_pipelineLayoutDescriptor);
            AZ_Assert(pipelineLayout, "PipelineLayout can not be null");
            
            const RHI::RenderAttachmentConfiguration& attachmentsConfiguration = descriptor.m_renderAttachmentConfiguration;
            m_renderPipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
            
            for (AZ::u32 i = 0; i < attachmentsConfiguration.GetRenderTargetCount(); ++i)
            {
                m_renderPipelineDesc.colorAttachments[i].pixelFormat = ConvertPixelFormat(attachmentsConfiguration.GetRenderTargetFormat(i));
                m_renderPipelineDesc.colorAttachments[i].writeMask = ConvertColorWriteMask(descriptor.m_renderStates.m_blendState.m_targets[i].m_writeMask);
                m_renderPipelineDesc.colorAttachments[i].blendingEnabled = descriptor.m_renderStates.m_blendState.m_targets[i].m_enable;
                m_renderPipelineDesc.colorAttachments[i].alphaBlendOperation = ConvertBlendOp(descriptor.m_renderStates.m_blendState.m_targets[i].m_blendAlphaOp);
                m_renderPipelineDesc.colorAttachments[i].rgbBlendOperation = ConvertBlendOp(descriptor.m_renderStates.m_blendState.m_targets[i].m_blendOp);
                m_renderPipelineDesc.colorAttachments[i].destinationAlphaBlendFactor = ConvertBlendFactor(descriptor.m_renderStates.m_blendState.m_targets[i].m_blendAlphaDest);
                m_renderPipelineDesc.colorAttachments[i].destinationRGBBlendFactor = ConvertBlendFactor(descriptor.m_renderStates.m_blendState.m_targets[i].m_blendDest);;
                m_renderPipelineDesc.colorAttachments[i].sourceAlphaBlendFactor = ConvertBlendFactor(descriptor.m_renderStates.m_blendState.m_targets[i].m_blendAlphaSource);
                m_renderPipelineDesc.colorAttachments[i].sourceRGBBlendFactor = ConvertBlendFactor(descriptor.m_renderStates.m_blendState.m_targets[i].m_blendSource);;
            }
            
            MTLVertexDescriptor* vertexDescriptor = [[MTLVertexDescriptor alloc] init];
            ConvertInputElements(descriptor.m_inputStreamLayout, vertexDescriptor);
            m_renderPipelineDesc.vertexDescriptor = vertexDescriptor;
            [vertexDescriptor release];
            vertexDescriptor = nil;
            
            m_renderPipelineDesc.vertexFunction = ExtractMtlFunction(device.GetMtlDevice(), descriptor.m_vertexFunction.get());
            AZ_Assert(m_renderPipelineDesc.vertexFunction, "Vertex mtlFuntion can not be null");
            m_renderPipelineDesc.fragmentFunction = ExtractMtlFunction(device.GetMtlDevice(), descriptor.m_fragmentFunction.get());
            
            RHI::Format depthStencilFormat = attachmentsConfiguration.GetDepthStencilFormat();
            if(descriptor.m_renderStates.m_depthStencilState.m_stencil.m_enable || IsDepthStencilMerged(depthStencilFormat))
            {
                m_renderPipelineDesc.stencilAttachmentPixelFormat = ConvertPixelFormat(depthStencilFormat);
            }
            
            //Depthstencil state
            if(descriptor.m_renderStates.m_depthStencilState.m_depth.m_enable || IsDepthStencilMerged(depthStencilFormat))
            {
                m_renderPipelineDesc.depthAttachmentPixelFormat = ConvertPixelFormat(depthStencilFormat);
                
                MTLDepthStencilDescriptor* depthStencilDesc = [[MTLDepthStencilDescriptor alloc] init];
                ConvertDepthStencilState(descriptor.m_renderStates.m_depthStencilState, depthStencilDesc);
                m_depthStencilState = [device.GetMtlDevice() newDepthStencilStateWithDescriptor:depthStencilDesc];
                AZ_Assert(m_depthStencilState, "Could not create Depth Stencil state.");
                [depthStencilDesc release];
                depthStencilDesc = nil;
            }
#if defined(__IPHONE_16_0) || defined(__MAC_13_0)
            m_renderPipelineDesc.rasterSampleCount = descriptor.m_renderStates.m_multisampleState.m_samples;
#else
            m_renderPipelineDesc.sampleCount = descriptor.m_renderStates.m_multisampleState.m_samples;
#endif
            
            m_renderPipelineDesc.alphaToCoverageEnabled = descriptor.m_renderStates.m_blendState.m_alphaToCoverageEnable;
            
            PipelineLibrary* pipelineLibrary = static_cast<PipelineLibrary*>(pipelineLibraryBase);
            if (pipelineLibrary && pipelineLibrary->IsInitialized())
            {
                m_graphicsPipelineState = pipelineLibrary->CreateGraphicsPipelineState(static_cast<uint64_t>(descriptor.GetHash()), m_renderPipelineDesc, &error);
            }
            else
            {
                MTLRenderPipelineReflection* ref;
                m_graphicsPipelineState = [device.GetMtlDevice() newRenderPipelineStateWithDescriptor:m_renderPipelineDesc options : MTLPipelineOptionBufferTypeInfo reflection : &ref error : &error];
            }
            
            AZ_Assert(m_graphicsPipelineState, "Could not create Pipeline object!.");
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
                [[maybe_unused]] const char * errorStr = [ error.localizedDescription UTF8String ];
                AZ_Error("PipelineState", false, "Failed to compile compute pipeline state with error: %s.", errorStr);
                return RHI::ResultCode::Fail;
            }
        }

        RHI::ResultCode PipelineState::InitInternal(RHI::Device& deviceBase,
                                                    const RHI::PipelineStateDescriptorForDispatch& descriptor,
                                                    RHI::SingleDevicePipelineLibrary* pipelineLibraryBase)
        {
            Device& device = static_cast<Device&>(deviceBase);
            NSError* error = 0;
            m_computePipelineDesc = [[MTLComputePipelineDescriptor alloc] init];
            RHI::ConstPtr<PipelineLayout> pipelineLayout = device.AcquirePipelineLayout(*descriptor.m_pipelineLayoutDescriptor);
            AZ_Assert(pipelineLayout, "PipelineLayout can not be null");
            m_computePipelineDesc.computeFunction = ExtractMtlFunction(device.GetMtlDevice(), descriptor.m_computeFunction.get());
            AZ_Assert(m_computePipelineDesc.computeFunction, "Compute mtlFuntion can not be null");
            
            PipelineLibrary* pipelineLibrary = static_cast<PipelineLibrary*>(pipelineLibraryBase);
            if (pipelineLibrary && pipelineLibrary->IsInitialized())
            {
                m_computePipelineState = pipelineLibrary->CreateComputePipelineState(static_cast<uint64_t>(descriptor.GetHash()), m_computePipelineDesc, &error);
            }
            else
            {
                MTLComputePipelineReflection* ref;
                m_computePipelineState = [device.GetMtlDevice() newComputePipelineStateWithDescriptor:m_computePipelineDesc options:MTLPipelineOptionBufferTypeInfo reflection:&ref error:&error];
            }
            
            AZ_Assert(m_computePipelineState, "Could not create Pipeline object!.");
            
            if (m_computePipelineState)
            {
                m_pipelineLayout = AZStd::move(pipelineLayout);
                return RHI::ResultCode::Success;
            }
            else
            {
                [[maybe_unused]] const char * errorStr = [ error.localizedDescription UTF8String ];
                AZ_Error("PipelineState", false, "Failed to compile compute pipeline state with error: %s.", errorStr);
                return RHI::ResultCode::Fail;
            }
        }

        RHI::ResultCode PipelineState::InitInternal(RHI::Device& device, const RHI::PipelineStateDescriptorForRayTracing& descriptor, RHI::SingleDevicePipelineLibrary* pipelineLibrary)
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
                [m_renderPipelineDesc release];
                m_renderPipelineDesc = nil;
                [m_graphicsPipelineState release];
                m_graphicsPipelineState = nil;
            }
            
            if (m_computePipelineState)
            {
                [m_computePipelineDesc release];
                m_computePipelineDesc = nil;
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
