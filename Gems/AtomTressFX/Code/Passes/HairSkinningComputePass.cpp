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

#include <Atom/RHI/CommandList.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/PipelineState.h>

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

// Hair Specific
#include <Rendering/HairCommon.h>
#include <Rendering/SharedBufferInterface.h>
#include <Rendering/HairDispatchItem.h>
#include <Rendering/HairFeatureProcessor.h>

#include <Passes/HairSkinningComputePass.h>

#pragma optimize("", off)

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {
            //        static const char* const HairSkinningBufferBaseName = "HairSkinningBuffer";
            Data::Instance<RPI::Shader> HairSkinningComputePass::GetShader()
            {
                return m_shader;
            }

            RPI::Ptr<HairSkinningComputePass> HairSkinningComputePass::Create(const RPI::PassDescriptor& descriptor)
            {
                RPI::Ptr<HairSkinningComputePass> pass = aznew HairSkinningComputePass(descriptor);
                return pass;
            }

            HairSkinningComputePass::HairSkinningComputePass(const RPI::PassDescriptor& descriptor)
                : RPI::ComputePass(descriptor)
            {
            }

            void HairSkinningComputePass::BuildAttachmentsInternal()
            {
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

                // Output
                // This is the buffer that is shared between all objects and dispatches and contains
                // the dynamic data that can be changed between passes.
                AttachBufferToSlot(Name{ "SkinnedHairSharedBuffer" }, m_featureProcessor->GetSharedBuffer());
             }

            // Currently this is an empty method as the Srgs are connected through the DispatchItem and
            // compiled according to need.
            void HairSkinningComputePass::CompileResources([[maybe_unused]] const RHI::FrameGraphCompileContext& context)
            {
                if (m_featureProcessor)
                {
                    /*
                    Data::Instance<RPI::AttachmentImage> linkedListHeadImage = m_featureProcessor->GetPerPixelHeadImage();
                    RHI::Image* image = linkedListHeadImage->GetRHIImage();
                    RHI::ImageViewDescriptor imageDesc = RHI::ImageViewDescriptor::Create(RHI::Format::R32_UINT, 0, 0);
                    Data::Instance<RHI::ImageView> imageView = image->GetImageView(imageDesc);
                    const RHI::Resource& imageResource = imageView->GetResource();
                    */
//                    Data::Instance<RPI::Buffer> linkedListBuffer = m_featureProcessor->GetPerPixelListBuffer();
//                    uint64_t bufferSize = linkedListBuffer->GetBufferSize();
//                    linkedListBuffer->ResetBufferData(0, bufferSize, 0);
                }

                // Compile the per pass srg - the rest of the srgs are compiled during DispatchItem creation
                if (m_shaderResourceGroup != nullptr)
                {
                    BindPassSrg(context, m_shaderResourceGroup);
                    m_shaderResourceGroup->Compile();
                }

                // Do we need to compile the srg?
//                AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
//                for (RHI::DispatchItem* dispatchItem : m_dispatchItems)
//                {
//
//                }
            }

            void HairSkinningComputePass::SetFeatureProcessor(HairFeatureProcessor* featureeProcessor)
            {
                m_featureProcessor = featureeProcessor;
                if (m_featureProcessor)
                {
                    m_shaderResourceGroup = featureeProcessor->GetPerPassSrg();
                }
            }

            bool HairSkinningComputePass::IsEnabled() const
            {
                return (m_dispatchItems.size() ? true : false);
            }

            void HairSkinningComputePass::AddDispatchItem(RHI::DispatchItem* dispatchItem)
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
                //using an unordered_set here to prevent redundantly adding the same dispatchItem to the submission queue
                //(i.e. if the same skinnedMesh exists in multiple views, it can call AddDispatchItem multiple times with the same item)
                m_dispatchItems.insert(dispatchItem);
            }

            void HairSkinningComputePass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
            {
                RHI::CommandList* commandList = context.GetCommandList();

                // The following will bind all registered Srgs set in m_shaderResourceGroupsToBind
                //  and sends them to the command list ahead of the dispatch.
                // Avoid doing that as we registered the srgs within the dispatchItem itself!
 //               SetSrgsForDispatch(commandList);    

                AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
                for (RHI::DispatchItem* dispatchItem : m_dispatchItems)
                {
                    commandList->Submit(*dispatchItem);
                }

                // Clear the dispatch items. They will need to be re-populated next frame
                m_dispatchItems.clear();
            }
            /*
            void HairSkinningComputePass::FrameBeginInternal(FramePrepareParams params)
            {
                AZ::RPI::ComputePass::FrameBeginInternal(params);

                if (!m_featureProcessor)
                {   // Assuming nothing was configured yet.
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
            */

        } // namespace Hair
    }   // namespace Render
}   // namespace AZ

#pragma optimize("", on)

