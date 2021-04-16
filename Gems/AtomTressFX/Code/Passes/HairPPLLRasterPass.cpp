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

//#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/DrawPacketBuilder.h>
#include <Atom/RHI/PipelineState.h>

#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/Scene.h>

#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Pass/RasterPassData.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderResourceGroupAsset.h>

#include <Passes/HairPPLLRasterPass.h>
#include <Rendering/HairRenderObject.h>
#include <Rendering/HairFeatureProcessor.h>

#pragma optimize("", off)
namespace AZ
{
    namespace Render
    {
        namespace Hair
        {

            // --- Creation & Initialization ---
            RPI::Ptr<HairPPLLRasterPass> HairPPLLRasterPass::Create(const RPI::PassDescriptor& descriptor)
            {
                RPI::Ptr<HairPPLLRasterPass> pass = aznew HairPPLLRasterPass(descriptor);
                return pass;
            }

            HairPPLLRasterPass::HairPPLLRasterPass(const RPI::PassDescriptor& descriptor)
                : RasterPass(descriptor),
                m_passDescriptor(descriptor)
            {
                // [To Do] Adi: add the buffers and clear here?
                LoadShaderAndPipelineState();
            }

            HairPPLLRasterPass::~HairPPLLRasterPass()
            {
            }

            /*
            bool HairPPLLRasterPass::Init()
            {
                if (m_initialized)
                {
                    return true;
                }
                return LoadShaderAndPipelineState();
            }
            */

            void HairPPLLRasterPass::RebuildPPLLBuffers()
            {
                if (!m_featureProcessor)
                {   // Assuming nothing was configured yet.
                    return;
                }
/*
                {   // reset the linked list buffer - this can be skipped once the image is being reset!
                    Data::Instance<RPI::Buffer> linkedListBuffer = m_featureProcessor->GetPerPixelListBuffer();
                    uint64_t bufferSize = linkedListBuffer->GetBufferSize();
                    linkedListBuffer->ResetBufferData(0, bufferSize, 0);
                }
*/
                {   // reset the linked list buffer - this can be skipped once the image is being reset!
                    Data::Instance<RPI::Buffer> linkedListCounterBuffer = m_featureProcessor->GetPerPixelCounterBuffer();
                    uint64_t bufferSize = linkedListCounterBuffer->GetBufferSize();
                    linkedListCounterBuffer->ResetBufferData(0, bufferSize, 0);
                }

                {   // Reset the PPLL head pointer image where each pixel contains index to the first
                    // PPLL structure of hair filling this pixel.
                    // Setting it to 0 is crucial for correct operation.
                    if (GetInputOutputCount() == 0)
                    {
                        AZ_Error("Hair Game", false, "Missing Input/Output Attachment in the Hair Compute Pass");
                        return;
                    }

                    RPI::PassAttachment* attachment = GetInputOutputBinding(0).m_attachment.get();
                    if (!attachment)
                    {
                        AZ_Error("Hair Game", false, "Input/Output binding in the Hair Compute pass has no attachment!");
                        return;
                    }

                    RHI::Size currentAttachmentSize = attachment->m_descriptor.m_image.m_size;
                    if (currentAttachmentSize == m_attachmentSize)
                    {   // No need for an update
                        return;
                    }

                    RHI::ImageDescriptor imageDesc = RHI::ImageDescriptor::Create2D(
                        RHI::ImageBindFlags::ShaderReadWrite, // GPU read and write is allowed
                        currentAttachmentSize.m_width, currentAttachmentSize.m_height, RHI::Format::R32_UINT
                    );

                    if (m_featureProcessor->CreateAndSetPerPixelHeadImage(imageDesc))
                    {
                        m_attachmentSize = currentAttachmentSize;
                    }
                }
            }

            void HairPPLLRasterPass::BuildAttachmentsInternal()
            {
                RebuildPPLLBuffers();

                if (!m_featureProcessor)
                {
                    RPI::Scene* scene = GetScene();
                    if (!scene)
                    {
                        return;
                    }

                    m_featureProcessor = scene->GetFeatureProcessor<HairFeatureProcessor>();
                    if (!m_featureProcessor)
                    {
                        return;
                    }
                }

                // Input
                // This is the buffer that is shared between all objects and dispatches and contains
                // the dynamic data that can be changed between passes.
                // NO need to define it - this is already defined by the Compute pass
//                AttachBufferToSlot(Name{ "SkinnedHairSharedBuffer" }, m_featureProcessor->GetSharedBuffer());

                // Output
                AttachBufferToSlot(Name{ "PPLLIndexCounter" }, m_featureProcessor->GetPerPixelCounterBuffer());
                AttachBufferToSlot(Name{ "PerPixelLinkedList" }, m_featureProcessor->GetPerPixelListBuffer());
                AttachImageToSlot(Name{ "PerPixelListHead" }, m_featureProcessor->GetPerPixelHeadImage());
            }

            bool HairPPLLRasterPass::IsEnabled() const
            {
                return m_initialized ? true : false;
            }

            void HairPPLLRasterPass::SetFeatureProcessor(HairFeatureProcessor* featureeProcessor)
            {
                m_featureProcessor = featureeProcessor;
                if (m_featureProcessor)
                {
                    m_shaderResourceGroup = featureeProcessor->GetPerPassSrg();
                }
            }

            bool HairPPLLRasterPass::LoadShaderAndPipelineState()
            {
                const RPI::RasterPassData* passData = RPI::PassUtils::GetPassData<RPI::RasterPassData>(m_passDescriptor);

                // If we successfully retrieved our custom data, use it to set the DrawListTag
                if (!passData)
                {
                    AZ_Error("Hair Gem", false, "Missing pass raster data");
                    return false;
                }

                // Load Shader
                const char* shaderFilePath = "Shaders/hairrenderingfillppll.azshader";
                Data::Asset<RPI::ShaderAsset> shaderAsset =
                    RPI::AssetUtils::LoadAssetByProductPath<RPI::ShaderAsset>(shaderFilePath, RPI::AssetUtils::TraceLevel::Error);

                if (!shaderAsset.GetId().IsValid())
                {
                    AZ_Error("Hair Gem", false, "Invalid shader asset for shader '%s'!", shaderFilePath);
                    return false;
                }

                m_shader = RPI::Shader::FindOrCreate(shaderAsset);
                if (m_shader == nullptr)
                {
                    AZ_Error("Hair Gem", false, "Pass failed to load shader '%s'!", shaderFilePath);
                    return false;
                }

                const RPI::ShaderVariant& shaderVariant = m_shader->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId);
                RHI::PipelineStateDescriptorForDraw pipelineStateDescriptor;
                shaderVariant.ConfigurePipelineState(pipelineStateDescriptor);

                RHI::DrawListTag drawListTag = m_shader->GetDrawListTag();
                RPI::Scene* scene = RPI::RPISystemInterface::Get()->GetDefaultScene().get();
                if (!scene)
                {
                    AZ_Error("Hair Gem", false, "Scene could not be acquired" );
                    return false;
                }
                scene->ConfigurePipelineState(drawListTag, pipelineStateDescriptor);

                pipelineStateDescriptor.m_inputStreamLayout.SetTopology(AZ::RHI::PrimitiveTopology::TriangleList);
                pipelineStateDescriptor.m_inputStreamLayout.Finalize();

                m_pipelineState = m_shader->AcquirePipelineState(pipelineStateDescriptor);
                if (!m_pipelineState)
                {
                    AZ_Error("Hair Gem", false, "Pipeline state could not be acquired");
                    return false;
                }
                m_initialized = true;
                return true;
            }


            // --- Pass behavior overrides ---
            /*
            void HairPPLLRasterPass::FrameBeginInternal(FramePrepareParams params)
            {
                // [To Do] Adi: add code here
                RasterPass::FrameBeginInternal(params);
            }
            */

            bool HairPPLLRasterPass::BuildDrawPacket(HairRenderObject* hairObject)
            {
                if (!m_shader || !m_pipelineState)
                {
                    return false;
                }
/*
                {
                    const RPI::ShaderVariant& shaderVariant = m_shader->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId);
                    RHI::PipelineStateDescriptorForDraw pipelineStateDescriptor;
                    shaderVariant.ConfigurePipelineState(pipelineStateDescriptor);

                    RHI::DrawListTag drawListTag = m_shader->GetDrawListTag();
                    RPI::Scene* scene = RPI::RPISystemInterface::Get()->GetDefaultScene().get();
                    if (!scene)
                    {
                        AZ_Error("Hair Gem", false, "Scene could not be acquired");
                        return false;
                    }
                    scene->ConfigurePipelineState(drawListTag, pipelineStateDescriptor);

                    pipelineStateDescriptor.m_inputStreamLayout.SetTopology(AZ::RHI::PrimitiveTopology::TriangleList);
                    pipelineStateDescriptor.m_inputStreamLayout.Finalize();

                    m_pipelineState = m_shader->AcquirePipelineState(pipelineStateDescriptor);
                    if (!m_pipelineState)
                    {
                        AZ_Error("Hair Gem", false, "Pipeline state could not be acquired");
                        return false;
                    }
                }
*/
                RHI::DrawPacketBuilder::DrawRequest drawRequest;
                drawRequest.m_listTag = m_drawListTag;
                drawRequest.m_pipelineState = m_pipelineState;
//                drawRequest.m_streamBufferViews =  // nor explicit vertex buffer.  shader is using the srg buffers
                drawRequest.m_stencilRef = 0;
                drawRequest.m_sortKey = 0;

                // Seems that the PerView and PerScene are gathered through RenderPass::CollectSrgs()
                // The PerPass is gathered through the RasterPass::m_shaderResourceGroup
                AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
                return hairObject->BuildPPLLDrawPacket(drawRequest);
            }


            bool HairPPLLRasterPass::AddDrawPacket(HairRenderObject* hairObject)
            {
                const RHI::DrawPacket* drawPacket = hairObject->GetFillDrawPacket();
                if (!drawPacket)
                {
                    AZ_Error("Hair Gem", false, "Failed to get or create the DrawPacket");
                        return false;
                }

                const RPI::ViewPtr currentView = GetView();

                // If this view is ignoring packets with our draw list tag then skip this view
                if (!currentView->HasDrawListTag(m_drawListTag))
                {
                    AZ_Warning("HairPPLLRasterPass", false, "Failed to match the DrawListTag");
                    return false;
                }

                AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
                currentView->AddDrawPacket(drawPacket);

                // [To Do] Adi: for now skip the next line.  IN the future possibly use the list ans
                // skip retrieval / creation.
//                    AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
//                    m_drawPackets.insert(drawPacket);

                return true;
            }

            
            void HairPPLLRasterPass::CompileResources(const RHI::FrameGraphCompileContext& context)
            {
                AZ_PROFILE_FUNCTION(Debug::ProfileCategory::Hair);

//                RPI::ShaderResourceGroup* perPass;
//                if (m_featureProcessor && (perPass = m_featureProcessor->GetPerPassSrg().get()))
//                {
//                    perPass->Compile();
//                }

                if (m_shaderResourceGroup != nullptr)
                {
                    BindPassSrg(context, m_shaderResourceGroup);
//                    m_shaderResourceGroup->Compile();
                }

//                context;
                // [To Do] Adi: add srg compilation here.

                //           BindPassSrg(context, m_shaderResourceGroup);
                //           m_shaderResourceGroup->Compile();
                //           Set Srgs into the Srgs list to bind here: m_shaderResourceGroupsToBind
                //             this can and should include the view Srg for retrieval of required data
                //             (instead of TressFXViewParams and set via GetViewLayout

//                RPI::RasterPass::CompileResources(context);
            }
            /*
            void HairPPLLRasterPass::FrameBeginInternal(FramePrepareParams params)
            {
                if (!m_featureProcessor)
                {   // Assuming nothing was configured yet.
                    RPI::RasterPass::FrameBeginInternal(params);
                    return;
                }

                {   // reset the linked list buffer - this can be skipped once the image is being reset!
                    Data::Instance<RPI::Buffer> linkedListBuffer = m_featureProcessor->GetPerPixelListBuffer();
                    uint64_t bufferSize = linkedListBuffer->GetBufferSize();
                    linkedListBuffer->ResetBufferData(0, bufferSize, 0);
                }

                {
                    if (GetInputOutputCount() == 0)
                    {
                        AZ_Error("Hair Game", false, "Missing Input/Output Attachment in the Hair Compute Pass");
                    }
                    else
                    {
                        RPI::PassAttachment* attachment = GetInputOutputBinding(0).m_attachment.get();
                        if (!attachment)
                        {
                            AZ_Error("Hair Game", false, "Input/Output binding in the Hair Compute pass has no attachment!");
                        }
                        else
                        {
                            RHI::Size currentAttachmentSize = attachment->m_descriptor.m_image.m_size;
                            if (currentAttachmentSize != m_attachmentSize)
                            {
                                RHI::ImageDescriptor imageDesc = RHI::ImageDescriptor::Create2D(
                                    RHI::ImageBindFlags::ShaderReadWrite, // GPU read and write is allowed
                                    currentAttachmentSize.m_width, currentAttachmentSize.m_height, RHI::Format::R32_UINT
                                );

                                if (m_featureProcessor->CreateAndSetPerPixelHeadImage(imageDesc))
                                {
                                    m_attachmentSize = currentAttachmentSize;
                                }
                            }
                        }
                    }
                }
                RPI::RasterPass::FrameBeginInternal(params);
            }
            */

            /*
            void HairPPLLRasterPass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
            {
                AZ_PROFILE_FUNCTION(Debug::ProfileCategory::Hair);

                RHI::CommandList* commandList = context.GetCommandList();

                if (!m_drawItems.empty())
                {
                    commandList->SetViewport(m_viewportState);
                    commandList->SetScissor(m_scissorState);

                    // [To Do] Adi: Verify all Srgs are in the list m_shaderResourceGroupsToBind at this stage
                    SetSrgsForDraw(commandList);
                }

                for (const AZ::RHI::DrawItem* drawItem : m_drawItems)
                {
                    commandList->Submit(*drawItem);
                }

                // remove all draw calls as prep for next frame.
                m_drawItems.clear();
            }
            */

        } // namespace Hair
    }   // namespace Render
}   // namespace AZ

#pragma optimize("", on)
