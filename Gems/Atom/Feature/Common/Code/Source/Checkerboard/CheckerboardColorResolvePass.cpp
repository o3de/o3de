/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/FrameGraphBuilder.h>

#include <Atom/RPI.Public/View.h>

#include <Checkerboard/CheckerboardColorResolvePass.h>
#include <Checkerboard/CheckerboardPass.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<Render::CheckerboardColorResolvePass> CheckerboardColorResolvePass::Create(const RPI::PassDescriptor& descriptor)
        {
            return aznew CheckerboardColorResolvePass(descriptor);
        }

        CheckerboardColorResolvePass::CheckerboardColorResolvePass(const RPI::PassDescriptor& descriptor)
            : Base(descriptor)
        {
        }

        void CheckerboardColorResolvePass::FrameBeginInternal(FramePrepareParams params)
        {
            // Import input attachments since some of them might be from last frame
            auto attachmentDatabase = params.m_frameGraphBuilder->GetAttachmentDatabase();
            for (const RPI::PassAttachmentBinding& binding : m_attachmentBindings)
            {
                if (binding.m_slotType == RPI::PassSlotType::Input && binding.m_attachment)
                {
                    auto& attachment = binding.m_attachment;
                    if (attachment->m_lifetime == RHI::AttachmentLifetimeType::Imported)
                    {
                        // make sure to only import the resource one time
                        RHI::AttachmentId attachmentId = attachment->GetAttachmentId();
                        if (!attachmentDatabase.IsAttachmentValid(attachmentId))
                        {
                            RPI::Image* image = azrtti_cast<RPI::Image*>(attachment->m_importedResource.get());
                            if (image)
                            {
                                attachmentDatabase.ImportImage(attachmentId, image->GetRHIImage());
                            }
                        }
                    }
                }
            }
            
            Base::FrameBeginInternal(params);
        }

        void CheckerboardColorResolvePass::BuildInternal()
        {
            // For each bound attachments they are the inputs from current frame.
            // We use them to get their owner CheckerboardPass then find the render targets from last frame
            // Then attach them to the slots for the inputs for previous frame.
            // Note: this requires the CheckerboardColorResolvePass input slots always has the order of a input color's current frame then previous frame.
            // For example:
            //      InputColor0_curr
            //      InputColor0_prev
            //      InputColor1_curr
            //      InputColor1_prev
            Data::Instance<RPI::AttachmentImage> inputImage;
            for (auto& binding : m_attachmentBindings)
            {
                if (binding.m_slotType == RPI::PassSlotType::Input)
                {
                    if (binding.m_attachment)
                    {
                        if (binding.m_attachment->m_ownerPass)
                        {
                            const RPI::Ptr<CheckerboardPass> checkerboardPass = azrtti_cast<CheckerboardPass*>(binding.m_attachment->m_ownerPass);
                            if (checkerboardPass)
                            {
                                inputImage = checkerboardPass->GetAttachmentImage(binding.m_attachment->m_name, 1);
                            }
                        }
                    }
                    else
                    {
                        AZ_Assert(inputImage, "this pass need matching orders of input slots");
                        AttachImageToSlot(binding.m_name, inputImage);
                        inputImage = nullptr;
                    }
                }
            }

            // reset frame offset to 0 since attachments are rebuilt
            m_frameOffset = 0;

            Base::BuildInternal();
        }

        void CheckerboardColorResolvePass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            // Same structure layout as PassSrg::Constants in CheckerboardColorResolveCS.azsl
            struct Constants
            {
                float m_frameOffset = 0;
                float m_depthTolerance = 2;
                uint32_t m_debugRenderFlags = 0;
                uint32_t m_checkOcclusion;
                Matrix4x4 m_prevClipToWorld;
                uint32_t m_numResolveTextures = 1;
                float m_padding[3];
            };

            Constants constants;
            constants.m_prevClipToWorld = m_prevClipToWorld;
            constants.m_frameOffset = m_frameOffset;
            constants.m_debugRenderFlags = static_cast<uint32_t> (m_debugRenderType);
            constants.m_checkOcclusion = m_checkOcclusion;
            constants.m_numResolveTextures = 3;

            // for next frame
            m_prevClipToWorld = GetView()->GetWorldToClipMatrix();
            m_prevClipToWorld.InvertFull();

            m_shaderResourceGroup->SetConstant(m_constantsIndex, constants);

            Base::CompileResources(context);
        }

        void CheckerboardColorResolvePass::FrameEndInternal()
        {
            // For the input slots for current frame, they always get updated when CheckerboardPass updates the render targets
            // But for the input slots for previous frame, we need to manually update them since they were manually attached in BuildInternal()
            //
            // When pass attachment was built, CheckerboardPass creates two resources for each render target.
            // For example, diffuse_0 and diffuse_1 which diffuse_0 is for even frame and diffuse_1 is for odd frame.
            //  - For the 2*N frame, the CheckerboardPass uses diffuse_0 for the output attachment, and it's InputColor0_curr of the CheckerboardColorResolvePass
            //    And we need to attach diffuse_1 for to InputColor0_prev.
            //  - For the 2*N+1 frame, the CheckerboardPass uses diffuse_1 for the output attachment, and diffuse_1 is the InputColor0_curr of the CheckerboardColorResolvePass
            //    because of the slot connection. So for InputColor0_prev slot, we need to use diffuse_0 as its input. 
            Data::Instance<RPI::AttachmentImage> nextAttachmentImage;
            for (auto& binding : m_attachmentBindings)
            {
                if (binding.m_slotType == RPI::PassSlotType::Input && binding.m_attachment)
                {
                    auto& attachment = binding.m_attachment;
                    // Use the input from current frame to find the owner CheckerboardPass
                    // then find the output of previous frame from CheckerboardPass
                    // Save the output in nextAttachmentImage and uses it in next binding (in the else condition)
                    if (attachment->m_ownerPass != this) // input from current frame
                    {
                        RPI::Ptr<CheckerboardPass> checkerboardPass = azrtti_cast<CheckerboardPass*>(binding.m_attachment->m_ownerPass);
                        nextAttachmentImage = checkerboardPass->GetAttachmentImage(binding.m_attachment->m_name, m_frameOffset);
                        AZ_Assert(nextAttachmentImage != binding.m_attachment->m_importedResource, "attachments shouldn't be same");
                    }
                    else // input from previous frame
                    {
                        AZ_Assert(nextAttachmentImage, "inputs should have correct order");

                        // update resource and attachment id for the pass attachment
                        attachment->m_path = nextAttachmentImage->GetAttachmentId();
                        attachment->m_importedResource = nextAttachmentImage;

                        for (auto& binding2 : m_attachmentBindings)
                        {
                            if (binding2.m_attachment == attachment)
                            {
                                binding2.m_unifiedScopeDesc.m_attachmentId = nextAttachmentImage->GetAttachmentId();
                                break;
                            }
                        }
                        nextAttachmentImage = nullptr;
                    }
                }
            }

            m_frameOffset = 1 - m_frameOffset;

            Base::FrameEndInternal();
        }
        
        void CheckerboardColorResolvePass::SetDebugRender(DebugRenderType type)
        {
            m_debugRenderType = type;
        }

        CheckerboardColorResolvePass::DebugRenderType CheckerboardColorResolvePass::GetDebugRenderType() const
        {
            return m_debugRenderType;
        }

        void CheckerboardColorResolvePass::SetEnableCheckOcclusion(bool enabled)
        {
            m_checkOcclusion = enabled;
        }

        bool CheckerboardColorResolvePass::IsCheckingOcclusion() const
        {
            return m_checkOcclusion;
        }

      } // namespace Render
} // namespace AZ
