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
            constexpr const char* verticalBlurShaderFilePath = "Shaders/Reflections/ReflectionScreenSpaceBlurVertical.azshader";
            Data::Instance<AZ::RPI::Shader> verticalBlurShader = RPI::LoadCriticalShader(verticalBlurShaderFilePath);
            if (verticalBlurShader == nullptr)
            {
                AZ_Error("PassSystem", false, "[ReflectionScreenSpaceBlurPass '%s']: Failed to load shader '%s'!", GetPathName().GetCStr(), verticalBlurShaderFilePath);
                return;
            }

            constexpr const char* horizontalBlurShaderFilePath = "Shaders/Reflections/ReflectionScreenSpaceBlurHorizontal.azshader";
            Data::Instance<AZ::RPI::Shader> horizontalBlurShader = RPI::LoadCriticalShader(horizontalBlurShaderFilePath);
            if (horizontalBlurShader == nullptr)
            {
                AZ_Error("PassSystem", false, "[ReflectionScreenSpaceBlurPass '%s']: Failed to load shader '%s'!", GetPathName().GetCStr(), horizontalBlurShaderFilePath);
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
            for (uint32_t mip = 0; mip < m_mipLevels - 1; ++mip)
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

            // retrieve the reflection attachment
            RPI::PassAttachment* reflectionImageAttachment = GetInputOutputBinding(0).GetAttachment().get();
            m_imageSize = reflectionImageAttachment->m_descriptor.m_image.m_size;
            m_mipLevels = aznumeric_cast<uint32_t>(reflectionImageAttachment->m_descriptor.m_image.m_mipLevels);

            // create transient attachments, one for each blur mip level
            AZStd::vector<RPI::PassAttachment*> transientPassAttachments;
            for (uint32_t mip = 1; mip <= m_mipLevels - 1; ++mip)
            {
                RHI::Size mipSize = m_imageSize.GetReducedMip(mip);

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
                RPI::PassAttachmentBinding& inputAttachmentBinding = verticalBlurChildPass->GetInputBinding(0);
                inputAttachmentBinding.SetAttachment(reflectionImageAttachment);
                inputAttachmentBinding.m_connectedBinding = &GetInputOutputBinding(0);

                // mipN transient output
                RPI::PassAttachmentBinding& outputAttachmentBinding = verticalBlurChildPass->GetOutputBinding(0);
                outputAttachmentBinding.SetAttachment(transientPassAttachments[attachmentIndex]);

                verticalBlurChildPass->UpdateConnectedBindings();

                attachmentIndex++;
            }
 
            // setup attachment bindings on horizontal blur child passes
            attachmentIndex = 0;
            for (auto& horizontalBlurChildPass : m_horizontalBlurChildPasses)
            {
                RPI::PassAttachmentBinding& inputAttachmentBinding = horizontalBlurChildPass->GetInputBinding(0);
                inputAttachmentBinding.SetAttachment(transientPassAttachments[attachmentIndex]);
                inputAttachmentBinding.m_connectedBinding = &m_verticalBlurChildPasses[attachmentIndex]->GetOutputBinding(0);

                RPI::PassAttachmentBinding& outputAttachmentBinding = horizontalBlurChildPass->GetOutputBinding(0);
                uint32_t mipLevel = attachmentIndex + 1;
                RHI::ImageViewDescriptor outputViewDesc;
                outputViewDesc.m_mipSliceMin = static_cast<int16_t>(mipLevel);
                outputViewDesc.m_mipSliceMax = static_cast<int16_t>(mipLevel);
                outputAttachmentBinding.m_unifiedScopeDesc.SetAsImage(outputViewDesc);
                outputAttachmentBinding.SetAttachment(reflectionImageAttachment);

                horizontalBlurChildPass->UpdateConnectedBindings();

                attachmentIndex++;
            }
        }

        void ReflectionScreenSpaceBlurPass::FrameBeginInternal(FramePrepareParams params)
        {
            // get input attachment size
            RPI::PassAttachment* inputAttachment = GetInputOutputBinding(0).GetAttachment().get();
            AZ_Assert(inputAttachment, "ReflectionScreenSpaceBlurChildPass: Input binding has no attachment!");
        
            RHI::Size size = inputAttachment->m_descriptor.m_image.m_size;
            if (m_imageSize != size)
            {
                m_imageSize = size;
        
                uint32_t mip = 1;
                for (auto& ownedAttachment : m_ownedAttachments)
                {
                    RHI::Size mipSize = m_imageSize.GetReducedMip(mip);
                    ownedAttachment->m_descriptor.m_image.m_size = mipSize;
                    mip++;
                }
            }
        
            ParentPass::FrameBeginInternal(params);
        }
    }   // namespace RPI
}   // namespace AZ
