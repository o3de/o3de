/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/FrameScheduler.h>
#include <Atom/RHI.Reflect/RenderAttachmentLayoutBuilder.h>

#include <Atom/RPI.Public/Buffer/BufferSystemInterface.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Pass/AttachmentReadback.h>
#include <Atom/RPI.Public/Pass/Specific/ImageAttachmentPreviewPass.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPIUtils.h>

#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>

namespace AZ
{
    namespace RPI
    {
        void ImageAttachmentCopy::SetImageAttachment(RHI::AttachmentId srcAttachmentId, RHI::AttachmentId destAttachmentId)
        {
            m_srcAttachmentId = srcAttachmentId;
            m_destAttachmentId = destAttachmentId;

            // Use the unique destination attachment id as scope id
            InitScope(m_destAttachmentId);

            // Clear the previous attachment and copy item
            m_copyItem = {};
            m_destImage = nullptr;
        }

        void ImageAttachmentCopy::Reset()
        {
            m_srcAttachmentId = RHI::AttachmentId{};
            m_destAttachmentId = RHI::AttachmentId{};
            m_copyItem = {};
            m_destImage = nullptr;
        }

        void ImageAttachmentCopy::InvalidateDestImage()
        {
            m_destImage = nullptr;
        }

        void ImageAttachmentCopy::FrameBegin(Pass::FramePrepareParams params)
        {
            RHI::FrameGraphAttachmentInterface attachmentDatabase = params.m_frameGraphBuilder->GetAttachmentDatabase();

            if (m_srcAttachmentId.IsEmpty())
            {
                return;
            }

            // Return if the source attachment is not imported
            if (!attachmentDatabase.IsAttachmentValid(m_srcAttachmentId))
            {
                Reset();
                return;
            }

            if (!m_destImage)
            {
                Data::Instance<AttachmentImagePool> pool = ImageSystemInterface::Get()->GetSystemAttachmentPool();
                RHI::ImageDescriptor imageDesc = attachmentDatabase.GetImageDescriptor(m_srcAttachmentId);
                // add read flag since the image will always be read by ImageAttachmentPreviewPass
                imageDesc.m_bindFlags |= RHI::ImageBindFlags::ShaderRead;
                imageDesc.m_arraySize = 1;

                Name copyName = Name( AZStd::string::format("%s_%s", m_srcAttachmentId.GetCStr(), "Copy") );
                m_destImage = AttachmentImage::Create(*pool.get(), imageDesc, copyName, nullptr, nullptr);
            }

            if (!m_destImage)
            {
                AZ_Warning("ImageAttachmentCopy", false, "Failed to create a copy to preview attachment [%s]", m_srcAttachmentId.GetCStr());
                Reset();
                return;
            }
            // Import this scope producer
            params.m_frameGraphBuilder->ImportScopeProducer(*this);
            attachmentDatabase.ImportImage(m_destAttachmentId, m_destImage->GetRHIImage());
        }

        void ImageAttachmentCopy::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            RHI::ImageScopeAttachmentDescriptor srcDescriptor{ m_srcAttachmentId };
            frameGraph.UseCopyAttachment(srcDescriptor, RHI::ScopeAttachmentAccess::Read);
            RHI::ImageScopeAttachmentDescriptor destDescriptor{ m_destAttachmentId };
            frameGraph.UseCopyAttachment(destDescriptor, RHI::ScopeAttachmentAccess::Write);

            frameGraph.SetEstimatedItemCount(1);
        }

        void ImageAttachmentCopy::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            // copy descriptor for copying image
            RHI::CopyImageDescriptor copyImage;
            const AZ::RHI::Image* image = context.GetImage(m_srcAttachmentId);
            copyImage.m_sourceImage = image;
            copyImage.m_sourceSize = image->GetDescriptor().m_size;
            copyImage.m_sourceSubresource.m_arraySlice = m_sourceArraySlice;
            copyImage.m_destinationImage = context.GetImage(m_destAttachmentId);
            
            m_copyItem = copyImage;
        }

        void ImageAttachmentCopy::BuildCommandList(const RHI::FrameGraphExecuteContext& context)
        {
            context.GetCommandList()->Submit(m_copyItem.GetDeviceCopyItem(context.GetDeviceIndex()));
        }

        Ptr<ImageAttachmentPreviewPass> ImageAttachmentPreviewPass::Create(const PassDescriptor& descriptor)
        {
            Ptr<ImageAttachmentPreviewPass> imageAttachmentPreviewPass = aznew ImageAttachmentPreviewPass(descriptor);
            return imageAttachmentPreviewPass;
        }

        ImageAttachmentPreviewPass::ImageAttachmentPreviewPass(const PassDescriptor& descriptor)
            : Pass(descriptor)
        {
            InitScope(RHI::ScopeId(GetPathName()));
        }

        ImageAttachmentPreviewPass::~ImageAttachmentPreviewPass()
        {
            Data::AssetBus::Handler::BusDisconnect();
        }

        void ImageAttachmentPreviewPass::PreviewImageAttachmentForPass(Pass* pass, const PassAttachment* passAttachment, RenderPipeline* previewOutputPipeline, u32 imageArraySlice)
        {
            if (passAttachment->GetAttachmentType() != RHI::AttachmentType::Image)
            {
                return;
            }

            ClearPreviewAttachment();
            
            // find the attachment in pass's attachment binding
            uint32_t bindingIndex = 0;
            for (auto& binding : pass->GetAttachmentBindings())
            {
                if (passAttachment == binding.GetAttachment())
                {
                    RHI::AttachmentId attachmentId = binding.GetAttachment()->GetAttachmentId();
                    
                    // Append slot index and pass name so the read back's name won't be same as the attachment used in other passes.
                    AZStd::string readbackName = AZStd::string::format("%s_%d_%s", attachmentId.GetCStr(),
                        bindingIndex, GetName().GetCStr());
                    m_attachmentCopy = AZStd::make_shared<AZ::RPI::ImageAttachmentCopy>();
                    m_attachmentCopy->SetImageAttachment(attachmentId, AZ::Name(readbackName));

                    pass->m_attachmentCopy = m_attachmentCopy;
                    break;
                }
                bindingIndex++;
            }

            if (bindingIndex == pass->GetAttachmentBindings().size())
            {
                AZ_Warning("RPI", false, "failed to find the attachment %s", passAttachment->GetAttachmentId().GetCStr());
                return;
            }

            m_updateDrawData = true;
            m_imageAttachmentId = m_attachmentCopy->m_destAttachmentId;
            m_attachmentCopy->m_sourceArraySlice = u16(imageArraySlice);

            // Set the output of this pass to write to the pipeline output
            if (!m_outputColorAttachment)
            {
                RenderPipeline* pipeline = previewOutputPipeline ? previewOutputPipeline : pass->GetRenderPipeline();
                if (pipeline)
                {
                    Pass* pipelinePass = pipeline->GetRootPass().get();
                    PassAttachmentBinding* binding = nullptr;

                    // Get either first output or first input/output
                    if (pipelinePass->GetOutputCount() > 0)
                    {
                        binding = &pipelinePass->GetOutputBinding(0);
                    }
                    else if (pipelinePass->GetInputOutputCount() > 0)
                    {
                        binding = &pipelinePass->GetInputOutputBinding(0);
                    }

                    if (binding)
                    {
                        SetOutputColorAttachment(binding->GetAttachment());
                    }

                    AZ_Warning("PassSystem", binding != nullptr, "ImageAttachmentPreviewPass couldn't find a color attachment on pipeline");
                }
            }
        }

        void ImageAttachmentPreviewPass::ClearPreviewAttachment()
        {
            ClearDrawData();
            // Allocate and release the copy scope only when there is an attachment to preview.
            // So we only need a weak ptr in the RenderPass and don't need to worry about releasing.
            m_attachmentCopy = nullptr;
            m_imageAttachmentId = RHI::AttachmentId{};
            m_updateDrawData = true;
            m_outputColorAttachment = nullptr;
        }
        
        void ImageAttachmentPreviewPass::SetPreviewLocation(AZ::Vector2 position, AZ::Vector2 size, bool keepAspectRatio)
        {
            m_position = position;
            m_size = size;
            m_keepAspectRatio = keepAspectRatio;
        }
        
        void ImageAttachmentPreviewPass::ClearDrawData()
        {
            if (m_needsShaderLoad)
            {
                return;
            }
            // update pass srg
            for (auto& previewInfo : m_imageTypePreviewInfo)
            {
                // unbind previous binded image views
                m_passSrg->SetImageView(previewInfo.m_imageInput, nullptr, 0);

                previewInfo.m_item.SetPipelineState(nullptr);
                previewInfo.m_imageCount = 0;
            }
            m_passSrgChanged = true;
        }

        void ImageAttachmentPreviewPass::SetOutputColorAttachment(RHI::Ptr<PassAttachment> outputImageAttachment)
        {
            m_outputColorAttachment = outputImageAttachment;
            m_updateDrawData = true;
        }

        void ImageAttachmentPreviewPass::OnAssetReloaded(Data::Asset<Data::AssetData> asset)
        {
            Data::Asset<ShaderAsset> shaderAsset = Data::static_pointer_cast<ShaderAsset>(asset);
            if (shaderAsset)
            {
                m_needsShaderLoad = true;
                m_updateDrawData = true;
            }
        }

        void ImageAttachmentPreviewPass::LoadShader()
        {
            m_needsShaderLoad = false;

            // Load Shader
            constexpr const char* ShaderPath = "shaders/imagepreview.azshader";
            Data::Asset<ShaderAsset> shaderAsset = RPI::FindShaderAsset(ShaderPath);
            if (!shaderAsset.IsReady())
            {
                AZ_Error("PassSystem", false, "[ImageAttachmentsPreviewPass]: Failed to load shader '%s'!", GetPathName().GetCStr(), ShaderPath);
                return;
            }

            m_shader = Shader::FindOrCreate(shaderAsset);
            if (m_shader == nullptr)
            {
                AZ_Error("PassSystem", false, "[ImageAttachmentsPreviewPass]: Failed to create shader instance from asset '%s'!", ShaderPath);
                return;
            }

            // Load SRG
            const auto srgLayout = m_shader->FindShaderResourceGroupLayout(SrgBindingSlot::Pass);
            if (srgLayout)
            {
                m_passSrg = ShaderResourceGroup::Create(shaderAsset, m_shader->GetSupervariantIndex(), srgLayout->GetName());

                if (!m_passSrg)
                {
                    AZ_Error("PassSystem", false, "Failed to create SRG from shader asset '%s'", ShaderPath);
                    return;
                }
            }

            // Find srg input indexes
            m_imageTypePreviewInfo[static_cast<uint32_t>(ImageType::Image2d)].m_imageInput = m_passSrg->FindShaderInputImageIndex(Name("m_image"));
            m_imageTypePreviewInfo[static_cast<uint32_t>(ImageType::Image2dMs)].m_imageInput = m_passSrg->FindShaderInputImageIndex(Name("m_msImage"));
            m_colorRangeMinMaxInput = m_passSrg->FindShaderInputConstantIndex(Name("m_colorRangeMinMax"));
            
            // Setup initial data for pipeline state descriptors. The rest of the data will be set when the draw data is updated

            // option names from the azsl file
            const char* optionValues[] =
            {
                "ImageType::Image2d",
                "ImageType::Image2dMs"
            };
            const char* optionName = "o_imageType";

            ShaderOptionGroup shaderOption = m_shader->CreateShaderOptionGroup();

            RHI::InputStreamLayout inputStreamLayout;
            inputStreamLayout.SetTopology(RHI::PrimitiveTopology::TriangleStrip);
            inputStreamLayout.Finalize();

            RHI::RenderAttachmentLayout attachmentsLayout;
            RHI::RenderAttachmentLayoutBuilder attachmentsLayoutBuilder;
            attachmentsLayoutBuilder.AddSubpass()
                ->RenderTargetAttachment(RHI::Format::R8G8B8A8_UNORM); // Set any format to avoid errors when building the layout.

            attachmentsLayoutBuilder.End(attachmentsLayout);

            for (uint32_t index = 0; index < static_cast<uint32_t>(ImageType::ImageTypeCount); index++)
            {
                ImageTypePreviewInfo& previewInfo = m_imageTypePreviewInfo[index];
                auto& pipelineDesc = previewInfo.m_pipelineStateDescriptor;

                shaderOption.SetValue(AZ::Name(optionName), AZ::Name(optionValues[index]));

                m_shader->GetVariant(shaderOption.GetShaderVariantId()).ConfigurePipelineState(pipelineDesc, shaderOption);
                pipelineDesc.m_renderAttachmentConfiguration.m_renderAttachmentLayout = attachmentsLayout;
                pipelineDesc.m_inputStreamLayout = inputStreamLayout;
                previewInfo.m_shaderVariantKeyFallback = shaderOption.GetShaderVariantKeyFallbackValue();
            }

            Data::AssetBus::Handler::BusDisconnect();
            Data::AssetBus::Handler::BusConnect(shaderAsset.GetId());
        }

        void ImageAttachmentPreviewPass::BuildInternal()
        {
            m_updateDrawData = true;
        }

        void ImageAttachmentPreviewPass::FrameBeginInternal(FramePrepareParams params)
        {
            bool scopeImported = false;
            if (!m_imageAttachmentId.IsEmpty() && m_outputColorAttachment)
            {
                // Only import the scope if the attachment is valid
                auto attachmentDatabase = params.m_frameGraphBuilder->GetAttachmentDatabase();
                bool isAttachmentValid = attachmentDatabase.IsAttachmentValid(m_imageAttachmentId);
                if (!isAttachmentValid)
                {
                    // Import the cached copy dest image if it exists (copied)
                    // So the attachment can be still previewed when the pass is disabled.
                    if (m_attachmentCopy && m_attachmentCopy->m_destImage)
                    {
                        attachmentDatabase.ImportImage(m_attachmentCopy->m_destAttachmentId, m_attachmentCopy->m_destImage->GetRHIImage());
                        isAttachmentValid = true;
                    }
                }

                if (isAttachmentValid)
                {
                    if (m_needsShaderLoad)
                    {
                        LoadShader();
                    }

                    params.m_frameGraphBuilder->ImportScopeProducer(*this);
                    scopeImported = true;
                }
            }

            // If the scope is not imported, we need compile the updated pass srg here
            if (m_passSrgChanged && !scopeImported)
            {
                m_passSrg->Compile();
                m_passSrgChanged = false;
            }
        }

        bool ImageAttachmentPreviewPass::ReadbackOutput(AZStd::shared_ptr<AttachmentReadback> readback)
        {
            if (m_outputColorAttachment)
            {
                m_readbackOption = PassAttachmentReadbackOption::Output;
                m_attachmentReadback = readback;
                AZStd::string readbackName = AZStd::string::format("%s_%s", m_outputColorAttachment->GetAttachmentId().GetCStr(), GetName().GetCStr());
                return m_attachmentReadback->ReadPassAttachment(m_outputColorAttachment.get(), AZ::Name(readbackName));
            }
            return false;
        }
        
        void ImageAttachmentPreviewPass::SetColorTransformRange(float colorTransformRange[2])
        {
            m_attachmentColorTranformRange[0] = AZ::GetMin(colorTransformRange[0], colorTransformRange[1]);
            m_attachmentColorTranformRange[1] = AZ::GetMax(colorTransformRange[0], colorTransformRange[1]);
            m_updateDrawData = true;
        }

        void ImageAttachmentPreviewPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            // add attachments to the scope
            // input attachment
            RHI::FrameGraphAttachmentInterface attachmentDatabase = frameGraph.GetAttachmentDatabase();
            RHI::ImageDescriptor imageDesc = attachmentDatabase.GetImageDescriptor(m_imageAttachmentId);
            // only preview mip 0 and array 0
            RHI::ImageViewDescriptor imageViewDesc = RHI::ImageViewDescriptor::Create(
                RHI::Format::Unknown, // no overwrite
                0,  //mipSliceMin
                0,  //mipSliceMax
                0,  //arraySliceMin
                0   //arraySliceMax
            );

            // If the format contains depth, set m_aspectFlags to depth, otherwise set it to color
            imageViewDesc.m_aspectFlags = RHI::CheckBitsAny(RHI::GetImageAspectFlags(imageDesc.m_format), RHI::ImageAspectFlags::Depth) ? 
                RHI::ImageAspectFlags::Depth : RHI::ImageAspectFlags::Color;

            auto scopeAttachmentDesc = RHI::ImageScopeAttachmentDescriptor{ m_imageAttachmentId, imageViewDesc};
            frameGraph.UseAttachment(scopeAttachmentDesc, RHI::ScopeAttachmentAccess::Read, RHI::ScopeAttachmentUsage::Shader, RHI::ScopeAttachmentStage::FragmentShader);

            // output attachment
            frameGraph.UseColorAttachment(RHI::ImageScopeAttachmentDescriptor{ m_outputColorAttachment->GetAttachmentId() });
            frameGraph.SetEstimatedItemCount(aznumeric_cast<uint32_t>(m_imageTypePreviewInfo.size()));
        }

        void ImageAttachmentPreviewPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            // setup srg data and draw item
            if (m_updateDrawData)
            {
                m_updateDrawData = false;

                // clear some old data
                ClearDrawData();
                
                ImageType imageType = ImageType::Unsupported;
                float aspectRatio = 1;

                // Find image type
                const RHI::ImageView* inputImageView = context.GetImageView(m_imageAttachmentId);
                if (!inputImageView)
                {
                    AZ_Warning("RPI", false, "Image attachment [%s] doesn't have image view in current context", m_imageAttachmentId.GetCStr());
                }
                else
                {
                    const RHI::ImageDescriptor& desc = inputImageView->GetImage()->GetDescriptor();
                    aspectRatio = desc.m_size.m_width / static_cast<float>(desc.m_size.m_height);

                    if (desc.m_dimension == RHI::ImageDimension::Image2D)
                    {
                        if (desc.m_multisampleState.m_samples == 1)
                        {
                            imageType = ImageType::Image2d;
                        }
                        else if (desc.m_multisampleState.m_samples > 1)
                        {
                            imageType = ImageType::Image2dMs;
                        }
                    }

                    if (imageType != ImageType::Unsupported)
                    {
                        const uint32_t typeIndex = static_cast<uint32_t>(imageType);
                        auto& previewInfo = m_imageTypePreviewInfo[typeIndex];
                        m_passSrg->SetShaderVariantKeyFallbackValue(previewInfo.m_shaderVariantKeyFallback);
                        m_passSrg->SetImageView(previewInfo.m_imageInput, inputImageView, 0);
                        m_passSrg->SetConstant(m_colorRangeMinMaxInput, m_attachmentColorTranformRange);
                        m_passSrgChanged = true;
                        previewInfo.m_imageCount = 1;
                    }
                    else
                    {
                        AZ_Warning("RPI", false, "Image attachment [%s] with format %d is not supported", m_imageAttachmentId.GetCStr(),
                            static_cast<uint32_t>(desc.m_format));
                    }
                }
                
                // config pipeline states related to output attachment
                const RHI::ImageView* outputImageView = context.GetImageView(m_outputColorAttachment->GetAttachmentId());
                RHI::Format outputFormat = outputImageView->GetDescriptor().m_overrideFormat;
                if (outputFormat == RHI::Format::Unknown)
                {
                    outputFormat = outputImageView->GetImage()->GetDescriptor().m_format;
                }
                
                // Base viewport and scissor off of output attachment
                RHI::Size targetImageSize = outputImageView->GetImage()->GetDescriptor().m_size;

                float width = m_size.GetX() * targetImageSize.m_width;
                float height = m_size.GetY() * targetImageSize.m_height;

                if (m_keepAspectRatio)
                {
                    if (width/height > aspectRatio)
                    {
                        width = height * aspectRatio;
                    }
                    else
                    {
                        height = width / aspectRatio;
                    }
                }

                float xMin = m_position.GetX() * targetImageSize.m_width;
                float yMin = m_position.GetY() * targetImageSize.m_height;

                m_viewport = RHI::Viewport(xMin, xMin + width, yMin, yMin + height);
                m_scissor = RHI::Scissor(0, 0, targetImageSize.m_width, targetImageSize.m_height);
                
                // compile
                if (m_passSrgChanged)
                {
                    m_passSrg->Compile();
                    m_passSrgChanged = false;
                }

                // rebuild draw item
                for (auto& previewInfo : m_imageTypePreviewInfo)
                {
                    if (previewInfo.m_imageCount == 0)
                    {
                        continue;
                    }
                    previewInfo.m_pipelineStateDescriptor.m_renderAttachmentConfiguration.m_renderAttachmentLayout.m_attachmentFormats[0] = outputFormat;
                    previewInfo.m_pipelineStateDescriptor.m_renderStates.m_multisampleState = outputImageView->GetImage()->GetDescriptor().m_multisampleState;

                    // draw each image by using instancing
                    RHI::DrawInstanceArguments drawInstanceArgs(previewInfo.m_imageCount, 0);
                    RHI::DrawLinear drawLinear(4, 0);

                    previewInfo.m_geometryView.SetDrawArguments(drawLinear);
                    previewInfo.m_item.SetDrawInstanceArgs(drawInstanceArgs);
                    previewInfo.m_item.SetGeometryView(&previewInfo.m_geometryView);
                    previewInfo.m_item.SetPipelineState(m_shader->AcquirePipelineState(previewInfo.m_pipelineStateDescriptor));
                }
            }
        }

        void ImageAttachmentPreviewPass::BuildCommandList(const RHI::FrameGraphExecuteContext& context)
        {
            RHI::CommandList* commandList = context.GetCommandList();

            commandList->SetViewport(m_viewport);
            commandList->SetScissor(m_scissor);

            // submit srg
            commandList->SetShaderResourceGroupForDraw(*m_passSrg->GetRHIShaderResourceGroup()->GetDeviceShaderResourceGroup(context.GetDeviceIndex()));

            // submit draw call
            for (uint32_t index = context.GetSubmitRange().m_startIndex; index < context.GetSubmitRange().m_endIndex; ++index)
            {
                const ImageTypePreviewInfo& previewInfo = m_imageTypePreviewInfo[index];
                if (previewInfo.m_imageCount > 0)
                {
                    commandList->Submit(previewInfo.m_item.GetDeviceDrawItem(context.GetDeviceIndex()), index);
                }
            }
        }
    }
}
