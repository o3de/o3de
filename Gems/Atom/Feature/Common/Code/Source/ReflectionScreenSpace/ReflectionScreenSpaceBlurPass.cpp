/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ReflectionScreenSpaceBlurPass.h"
#include "ReflectionScreenSpaceBlurChildPass.h"
#include <Atom/RHI/FrameGraphBuilder.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI.Reflect/ImageViewDescriptor.h>
#include <Atom/RPI.Reflect/Pass/FullscreenTrianglePassData.h>
#include <Atom/RPI.Reflect/Pass/PassName.h>
#include <Atom/RPI.Public/Pass/PassDefines.h>
#include <Atom/RPI.Public/Pass/PassFactory.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Reflect/Pass/PassRequest.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Image/AttachmentImagePool.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<ReflectionScreenSpaceBlurPass> ReflectionScreenSpaceBlurPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<ReflectionScreenSpaceBlurPass> pass = aznew ReflectionScreenSpaceBlurPass(descriptor);
            return AZStd::move(pass);
        }

        ReflectionScreenSpaceBlurPass::ReflectionScreenSpaceBlurPass(const RPI::PassDescriptor& descriptor)
            : RPI::ParentPass(descriptor)
        {
        }

        void ReflectionScreenSpaceBlurPass::ResetInternal()
        {
            RemoveChildren();
        }

        void ReflectionScreenSpaceBlurPass::CreateChildPassesInternal()
        {
            RPI::PassSystemInterface* passSystem = RPI::PassSystemInterface::Get();

            m_verticalBlurChildPasses.clear();
            m_horizontalBlurChildPasses.clear();

            // load shaders
            AZStd::string verticalBlurShaderFilePath = "Shaders/Reflections/ReflectionScreenSpaceBlurVertical.azshader";
            Data::Instance<AZ::RPI::Shader> verticalBlurShader = RPI::LoadCriticalShader(verticalBlurShaderFilePath);
            if (verticalBlurShader == nullptr)
            {
                AZ_Error("PassSystem", false, "[ReflectionScreenSpaceBlurPass '%s']: Failed to load shader '%s'!", GetPathName().GetCStr(), verticalBlurShaderFilePath.c_str());
                return;
            }

            AZStd::string horizontalBlurShaderFilePath = "Shaders/Reflections/ReflectionScreenSpaceBlurHorizontal.azshader";
            Data::Instance<AZ::RPI::Shader> horizontalBlurShader = RPI::LoadCriticalShader(horizontalBlurShaderFilePath);
            if (horizontalBlurShader == nullptr)
            {
                AZ_Error("PassSystem", false, "[ReflectionScreenSpaceBlurPass '%s']: Failed to load shader '%s'!", GetPathName().GetCStr(), horizontalBlurShaderFilePath.c_str());
                return;
            }

            // load pass templates
            const AZStd::shared_ptr<const RPI::PassTemplate> blurVerticalPassTemplate = RPI::PassSystemInterface::Get()->GetPassTemplate(Name("ReflectionScreenSpaceBlurVerticalPassTemplate"));
            const AZStd::shared_ptr<const RPI::PassTemplate> blurHorizontalPassTemplate = RPI::PassSystemInterface::Get()->GetPassTemplate(Name("ReflectionScreenSpaceBlurHorizontalPassTemplate"));

            // create pass descriptors
            RPI::PassDescriptor verticalBlurChildDesc;
            verticalBlurChildDesc.m_passTemplate = blurVerticalPassTemplate;

            RPI::PassDescriptor horizontalBlurChildDesc;
            horizontalBlurChildDesc.m_passTemplate = blurHorizontalPassTemplate;

            // add child passes to perform the vertical and horizontal Gaussian blur for each roughness mip level
            for (uint32_t mip = 0; mip < NumMipLevels - 1; ++mip)
            {
                // create Vertical blur child passes
                {
                    AZStd::string verticalBlurChildPassName = AZStd::string::format("ReflectionScreenSpace_VerticalMipBlur%d", mip + 1);
                    verticalBlurChildDesc.m_passName = Name(verticalBlurChildPassName);

                    RPI::Ptr<Render::ReflectionScreenSpaceBlurChildPass> verticalBlurChildPass = passSystem->CreatePass<Render::ReflectionScreenSpaceBlurChildPass>(verticalBlurChildDesc);
                    verticalBlurChildPass->SetType(Render::ReflectionScreenSpaceBlurChildPass::PassType::Vertical);
                    verticalBlurChildPass->SetMipLevel(mip + 1);
                    m_verticalBlurChildPasses.push_back(verticalBlurChildPass);

                    AddChild(verticalBlurChildPass);
                }

                // create Horizontal blur child passes
                {
                    AZStd::string horizontalBlurChildPassName = AZStd::string::format("ReflectionScreenSpace_HorizonalMipBlur%d", mip + 1);
                    horizontalBlurChildDesc.m_passName = Name(horizontalBlurChildPassName);

                    RPI::Ptr<Render::ReflectionScreenSpaceBlurChildPass> horizontalBlurChildPass = passSystem->CreatePass<Render::ReflectionScreenSpaceBlurChildPass>(horizontalBlurChildDesc);
                    horizontalBlurChildPass->SetType(Render::ReflectionScreenSpaceBlurChildPass::PassType::Horizontal);
                    horizontalBlurChildPass->SetMipLevel(mip + 1);
                    m_horizontalBlurChildPasses.push_back(horizontalBlurChildPass);

                    AddChild(horizontalBlurChildPass);
                }
            }
        }

        void ReflectionScreenSpaceBlurPass::BuildInternal()
        {
            RemoveChildren();
            m_flags.m_createChildren = true;

            // retrieve the reflection, downsampled normal, and downsampled depth attachments
            RPI::PassAttachment* reflectionImageAttachment = GetInputOutputBinding(0).GetAttachment().get();
            RHI::Size imageSize = reflectionImageAttachment->m_descriptor.m_image.m_size;

            RPI::PassAttachment* downsampledDepthImageAttachment = GetInputOutputBinding(1).GetAttachment().get();

            // create transient attachments, one for each blur mip level
            AZStd::vector<RPI::PassAttachment*> transientPassAttachments;
            for (uint32_t mip = 1; mip <= NumMipLevels - 1; ++mip)
            {
                RHI::Size mipSize = imageSize.GetReducedMip(mip);

                RHI::ImageBindFlags imageBindFlags = RHI::ImageBindFlags::Color | RHI::ImageBindFlags::ShaderReadWrite | RHI::ImageBindFlags::CopyRead;
                auto transientImageDesc = RHI::ImageDescriptor::Create2D(imageBindFlags, mipSize.m_width, mipSize.m_height, RHI::Format::R16G16B16A16_FLOAT);

                RPI::PassAttachment* transientPassAttachment = aznew RPI::PassAttachment();
                AZStd::string transientAttachmentName = AZStd::string::format("%s.ReflectionScreenSpace_BlurImage%d", GetPathName().GetCStr(), mip);
                transientPassAttachment->m_name = transientAttachmentName;
                transientPassAttachment->m_path = transientAttachmentName;
                transientPassAttachment->m_lifetime = RHI::AttachmentLifetimeType::Transient;
                transientPassAttachment->m_descriptor = transientImageDesc;
                transientPassAttachments.push_back(transientPassAttachment);

                m_ownedAttachments.push_back(transientPassAttachment);
            }

            // call ParentPass::BuildInternal() first to configure the slots and auto-add the empty bindings,
            // then we will assign attachments to the bindings
            ParentPass::BuildInternal();

            // setup attachment bindings on vertical blur child passes
            uint32_t attachmentIndex = 0;
            for (auto& verticalBlurChildPass : m_verticalBlurChildPasses)
            {
                // mip0 source input
                RPI::PassAttachmentBinding& inputAttachmentBinding = verticalBlurChildPass->GetInputOutputBinding(0);
                inputAttachmentBinding.SetAttachment(reflectionImageAttachment);
                inputAttachmentBinding.m_connectedBinding = &GetInputOutputBinding(0);

                // mipN transient output
                RPI::PassAttachmentBinding& outputAttachmentBinding = verticalBlurChildPass->GetInputOutputBinding(1);
                outputAttachmentBinding.SetAttachment(transientPassAttachments[attachmentIndex]);

                // setup downsampled depth output
                // Note: this is a vertical pass output only, and each vertical child pass writes a specific mip level
                uint32_t mipLevel = attachmentIndex + 1;

                // downsampled depth output
                RPI::PassAttachmentBinding& downsampledDepthAttachmentBinding = verticalBlurChildPass->GetInputOutputBinding(2);
                RHI::ImageViewDescriptor downsampledDepthOutputViewDesc;
                downsampledDepthOutputViewDesc.m_mipSliceMin = static_cast<int16_t>(mipLevel);
                downsampledDepthOutputViewDesc.m_mipSliceMax = static_cast<int16_t>(mipLevel);
                downsampledDepthAttachmentBinding.m_unifiedScopeDesc.SetAsImage(downsampledDepthOutputViewDesc);
                downsampledDepthAttachmentBinding.SetAttachment(downsampledDepthImageAttachment);

                attachmentIndex++;
            }
 
            // setup attachment bindings on horizontal blur child passes
            attachmentIndex = 0;
            for (auto& horizontalBlurChildPass : m_horizontalBlurChildPasses)
            {
                RPI::PassAttachmentBinding& inputAttachmentBinding = horizontalBlurChildPass->GetInputOutputBinding(0);
                inputAttachmentBinding.SetAttachment(transientPassAttachments[attachmentIndex]);

                RPI::PassAttachmentBinding& outputAttachmentBinding = horizontalBlurChildPass->GetInputOutputBinding(1);
                uint32_t mipLevel = attachmentIndex + 1;
                RHI::ImageViewDescriptor outputViewDesc;
                outputViewDesc.m_mipSliceMin = static_cast<int16_t>(mipLevel);
                outputViewDesc.m_mipSliceMax = static_cast<int16_t>(mipLevel);
                outputAttachmentBinding.m_unifiedScopeDesc.SetAsImage(outputViewDesc);
                outputAttachmentBinding.SetAttachment(reflectionImageAttachment);

                attachmentIndex++;
            }
        }
    }   // namespace RPI
}   // namespace AZ
