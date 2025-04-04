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
#include <Atom/RHI/Factory.h>
#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <RHI/PipelineState.h>
#include <RHI/PipelineLibrary.h>
#include <AzCore/std/algorithm.h>

namespace AZ
{
    namespace Metal
    {
        void RasterizerState::UpdateHash()
        {
            HashValue64 seed = HashValue64{ 0 };
            seed = TypeHash64(m_cullMode, seed);
            seed = TypeHash64(m_depthBias, seed);
            seed = TypeHash64(m_depthSlopeScale, seed);
            seed = TypeHash64(m_depthBiasClamp, seed);
            seed = TypeHash64(m_frontFaceWinding, seed);
            seed = TypeHash64(m_triangleFillMode, seed);
            seed = TypeHash64(m_depthClipMode, seed);
            m_hash = seed;
        }
    
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
        
        id<MTLFunction> PipelineState::CompileShader(id<MTLDevice> mtlDevice, const AZStd::string_view sourceStr, const AZStd::string_view entryPoint, const ShaderStageFunction* shaderFunction, MTLFunctionConstantValues* constantValues)
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
                dispatch_release(dispatchByteCodeData);
            }
            else
            {
                if(!sourceStr.empty())
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
                else
                {
                    AZ_Assert(false, "Shader source is not added by default. It can be added by enabling /O3DE/Atom/RHI/GraphicsDevMode via settings registry and re-building the shader.");
                }
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
                pFunction = [lib newFunctionWithName:entryPointStr constantValues:constantValues error:&error];
                [entryPointStr release];
                entryPointStr = nil;
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
                                                    RHI::DevicePipelineLibrary* pipelineLibraryBase)
        {
            NSError* error = 0;
            Device& device = static_cast<Device&>(deviceBase);
            RHI::ConstPtr<PipelineLayout> pipelineLayout = device.AcquirePipelineLayout(*descriptor.m_pipelineLayoutDescriptor);
            AZ_Assert(pipelineLayout, "PipelineLayout can not be null");
            
            const RHI::RenderAttachmentConfiguration& attachmentsConfiguration = descriptor.m_renderAttachmentConfiguration;
            m_renderPipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
            
            const auto& attachmentLayout = attachmentsConfiguration.m_renderAttachmentLayout;
            RHI::Format depthStencilFormat = RHI::Format::Unknown;
            m_attachmentIndexToColorIndex.fill(-1);
            int currentColorIndex = 0;
            // Initialize all used color attachments (by all subpasses) and only enable the ones used in this subpass.
            // Also collect the format for the depth/stencil if one is being used.
            for (uint32_t subpassIndex = 0; subpassIndex < attachmentLayout.m_subpassCount; ++subpassIndex)
            {
                const RHI::SubpassRenderAttachmentLayout& subpassLayout = attachmentLayout.m_subpassLayouts[subpassIndex];
                if(depthStencilFormat == RHI::Format::Unknown && subpassLayout.m_depthStencilDescriptor.IsValid())
                {
                    depthStencilFormat = attachmentLayout.m_attachmentFormats[subpassLayout.m_depthStencilDescriptor.m_attachmentIndex];
                }
                
                for(uint32_t subpassAttachmentIndex = 0; subpassAttachmentIndex < subpassLayout.m_rendertargetCount; ++subpassAttachmentIndex)
                {
                    uint32_t i = subpassLayout.m_rendertargetDescriptors[subpassAttachmentIndex].m_attachmentIndex;
                    if (m_attachmentIndexToColorIndex[i] < 0)
                    {
                        m_attachmentIndexToColorIndex[i] = currentColorIndex++;
                    }
                    MTLRenderPipelineColorAttachmentDescriptor* colorDescriptor = m_renderPipelineDesc.colorAttachments[m_attachmentIndexToColorIndex[i]];
                    colorDescriptor.pixelFormat = ConvertPixelFormat(attachmentLayout.m_attachmentFormats[i]);
                    colorDescriptor.writeMask = MTLColorWriteMaskNone;
                    colorDescriptor.blendingEnabled = false;
                }
            }

            // Enable the color attachments used by the subpass.
            // Also translates the proper color index for the blending state (since index 0 may now be index 3
            // due to merging passes)
            const RHI::SubpassRenderAttachmentLayout& subpassLayout = attachmentLayout.m_subpassLayouts[attachmentsConfiguration.m_subpassIndex];
            for(uint32_t subpassAttachmentIndex = 0; subpassAttachmentIndex < subpassLayout.m_rendertargetCount; ++subpassAttachmentIndex)
            {
                const auto& blendState = descriptor.m_renderStates.m_blendState.m_targets[subpassAttachmentIndex];
                uint32_t i = subpassLayout.m_rendertargetDescriptors[subpassAttachmentIndex].m_attachmentIndex;
                MTLRenderPipelineColorAttachmentDescriptor* colorDescriptor = m_renderPipelineDesc.colorAttachments[m_attachmentIndexToColorIndex[i]];
                colorDescriptor.pixelFormat = ConvertPixelFormat(attachmentLayout.m_attachmentFormats[i]);
                colorDescriptor.writeMask = ConvertColorWriteMask(blendState.m_writeMask);
                colorDescriptor.blendingEnabled = blendState.m_enable;
                colorDescriptor.alphaBlendOperation = ConvertBlendOp(blendState.m_blendAlphaOp);
                colorDescriptor.rgbBlendOperation = ConvertBlendOp(blendState.m_blendOp);
                colorDescriptor.destinationAlphaBlendFactor = ConvertBlendFactor(blendState.m_blendAlphaDest);
                colorDescriptor.destinationRGBBlendFactor = ConvertBlendFactor(blendState.m_blendDest);;
                colorDescriptor.sourceAlphaBlendFactor = ConvertBlendFactor(blendState.m_blendAlphaSource);
                colorDescriptor.sourceRGBBlendFactor = ConvertBlendFactor(blendState.m_blendSource);
            }
            
            MTLVertexDescriptor* vertexDescriptor = [[MTLVertexDescriptor alloc] init];
            ConvertInputElements(descriptor.m_inputStreamLayout, vertexDescriptor);
            m_renderPipelineDesc.vertexDescriptor = vertexDescriptor;
            [vertexDescriptor release];
            vertexDescriptor = nil;
            
            MTLFunctionConstantValues* constantValues = CreateFunctionConstantsValues(descriptor);
            
            m_renderPipelineDesc.vertexFunction = ExtractMtlFunction(device.GetMtlDevice(), descriptor.m_vertexFunction.get(), constantValues);
            AZ_Assert(m_renderPipelineDesc.vertexFunction, "Vertex mtlFuntion can not be null");
            m_renderPipelineDesc.fragmentFunction = ExtractMtlFunction(device.GetMtlDevice(), descriptor.m_fragmentFunction.get(), constantValues);
            
            if(descriptor.m_renderStates.m_depthStencilState.m_stencil.m_enable || IsDepthStencilMerged(depthStencilFormat))
            {
                m_renderPipelineDesc.stencilAttachmentPixelFormat = ConvertPixelFormat(depthStencilFormat);
            }
            
            // Depthstencil state
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
            
            if(m_graphicsPipelineState==nil)
            {
                if (RHI::Validation::IsEnabled())
                {
                    NSLog(@" error => %@ ", [error userInfo] );
                }
                AZ_Assert(false, "Could not create Pipeline object!.");
            }
            //We keep the descriptors alive in case we want to build the PSO cache. Delete them otherwise.
            if (!r_enablePsoCaching)
            {
                [m_renderPipelineDesc release];
                m_renderPipelineDesc = nil;
            }
            
            [constantValues release];
            constantValues = nil;
             
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
                                                    RHI::DevicePipelineLibrary* pipelineLibraryBase)
        {
            Device& device = static_cast<Device&>(deviceBase);
            NSError* error = 0;
            m_computePipelineDesc = [[MTLComputePipelineDescriptor alloc] init];
            RHI::ConstPtr<PipelineLayout> pipelineLayout = device.AcquirePipelineLayout(*descriptor.m_pipelineLayoutDescriptor);
            AZ_Assert(pipelineLayout, "PipelineLayout can not be null");
            MTLFunctionConstantValues* constantValues = CreateFunctionConstantsValues(descriptor);
            m_computePipelineDesc.computeFunction = ExtractMtlFunction(device.GetMtlDevice(), descriptor.m_computeFunction.get(), constantValues);
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
            
            //We keep the descriptors alive in case we want to build the PSO cache. Delete them otherwise.
            if (!r_enablePsoCaching)
            {
                [m_computePipelineDesc release];
                m_computePipelineDesc = nil;
            }
                                                                       
            [constantValues release];
            constantValues = nil;
            
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

        RHI::ResultCode PipelineState::InitInternal(RHI::Device& device, const RHI::PipelineStateDescriptorForRayTracing& descriptor, RHI::DevicePipelineLibrary* pipelineLibrary)
        {
            // [GFX TODO][ATOM-5268] Implement Metal Ray Tracing
            AZ_Assert(false, "Not implemented");
            return RHI::ResultCode::Fail;
        }

        id<MTLFunction> PipelineState::ExtractMtlFunction(id<MTLDevice> mtlDevice, const RHI::ShaderStageFunction* stageFunc, MTLFunctionConstantValues* constantValues)
        {
            // set the bound shader state settings
            if (stageFunc)
            {
                const ShaderStageFunction* shaderFunction = azrtti_cast<const ShaderStageFunction*>(stageFunc);
                AZStd::string_view strView(shaderFunction->GetSourceCode());
                
                id<MTLFunction> mtlFunction = nil;
                mtlFunction = CompileShader(mtlDevice, strView, shaderFunction->GetEntryFunctionName(), shaderFunction, constantValues);

                return mtlFunction;
            }
            
            return nil;
        }
    
        MTLFunctionConstantValues* PipelineState::CreateFunctionConstantsValues(const RHI::PipelineStateDescriptor& pipelineDescriptor) const
        {
            MTLFunctionConstantValues* constantValues = [[MTLFunctionConstantValues alloc] init];
            for(const auto& specializationData : pipelineDescriptor.m_specializationData)
            {
                uint32_t value = specializationData.m_value.GetIndex();
                MTLDataType type;
                switch(specializationData.m_type)
                {
                    case RHI::SpecializationType::Integer:
                        type = MTLDataTypeInt;
                        break;
                    case RHI::SpecializationType::Bool:
                        type = MTLDataTypeBool;
                        break;
                    default:
                        AZ_Assert(false, "Invalid specialization type %d", specializationData.m_type);
                        type = MTLDataTypeInt;
                        break;
                }
                [constantValues setConstantValue:&value type:type atIndex:specializationData.m_id];
            }
            
            if(const RHI::PipelineStateDescriptorForDraw* drawPipelineDescriptor = azrtti_cast<const RHI::PipelineStateDescriptorForDraw*>(&pipelineDescriptor))
            {
                // Set the function specialization values for the color and input attachments according to the attachments of the renderpass.
                // This is needed in order to support merging of passes as subpasses. See Metal::ShaderPlatformInterface
                const RHI::RenderAttachmentConfiguration& renderAttachmentConfiguration = drawPipelineDescriptor->m_renderAttachmentConfiguration;
                const RHI::SubpassRenderAttachmentLayout& subpassLayout = renderAttachmentConfiguration.m_renderAttachmentLayout.m_subpassLayouts[renderAttachmentConfiguration.m_subpassIndex];
                for (uint32_t i = 0; i < subpassLayout.m_rendertargetCount; ++i)
                {
                    uint32_t value = m_attachmentIndexToColorIndex[subpassLayout.m_rendertargetDescriptors[i].m_attachmentIndex];
                    NSString* functionConstantName = [[NSString alloc] initWithCString : AZStd::string::format("colorAttachment%d", i).c_str() encoding: NSASCIIStringEncoding];
                    [constantValues setConstantValue:&value type:MTLDataTypeInt withName:functionConstantName];
                    [functionConstantName  release];
                    functionConstantName  = nil;
                }
                for (uint32_t i = 0; i < subpassLayout.m_subpassInputCount; ++i)
                {
                    uint32_t value = m_attachmentIndexToColorIndex[subpassLayout.m_subpassInputDescriptors[i].m_attachmentIndex];
                    NSString* functionConstantName = [[NSString alloc] initWithCString : AZStd::string::format("inputAttachment%d", i).c_str() encoding: NSASCIIStringEncoding];
                    [constantValues setConstantValue:&value type:MTLDataTypeInt withName:functionConstantName];
                    [functionConstantName  release];
                    functionConstantName  = nil;
                }
            }
            return constantValues;
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
