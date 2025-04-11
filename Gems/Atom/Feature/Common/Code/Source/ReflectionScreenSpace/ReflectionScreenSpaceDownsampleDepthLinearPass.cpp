/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ReflectionScreenSpaceDownsampleDepthLinearPass.h"
#include "ReflectionScreenSpaceDownsampleDepthLinearChildPass.h"
#include <Atom/RPI.Public/RPIUtils.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<ReflectionScreenSpaceDownsampleDepthLinearPass> ReflectionScreenSpaceDownsampleDepthLinearPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<ReflectionScreenSpaceDownsampleDepthLinearPass> pass = aznew ReflectionScreenSpaceDownsampleDepthLinearPass(descriptor);
            return AZStd::move(pass);
        }

        ReflectionScreenSpaceDownsampleDepthLinearPass::ReflectionScreenSpaceDownsampleDepthLinearPass(const RPI::PassDescriptor& descriptor)
            : RPI::ParentPass(descriptor)
        {
        }

        void ReflectionScreenSpaceDownsampleDepthLinearPass::ResetInternal()
        {
            RemoveChildren();
        }

        void ReflectionScreenSpaceDownsampleDepthLinearPass::CreateChildPassesInternal()
        {
            RPI::PassSystemInterface* passSystem = RPI::PassSystemInterface::Get();

            // load shader
            constexpr const char* shaderFilePath = "Shaders/Reflections/ReflectionScreenSpaceDownsampleDepthLinear.azshader";
            Data::Instance<AZ::RPI::Shader> shader = RPI::LoadCriticalShader(shaderFilePath);
            if (shader == nullptr)
            {
                AZ_Error("PassSystem", false, "[ReflectionScreenSpaceDownsampleDepthLinearPass '%s']: Failed to load shader '%s'!", GetPathName().GetCStr(), shaderFilePath);
                return;
            }

            // create pass descriptor
            RPI::PassDescriptor childPassDescriptor;
            childPassDescriptor.m_passTemplate = RPI::PassSystemInterface::Get()->GetPassTemplate(Name("ReflectionScreenSpaceDownsampleDepthLinearChildPassTemplate"));;

            // add child passes to perform the downsample for each mip level
            for (uint32_t mip = 0; mip < m_mipLevels; ++mip)
            {
                AZStd::string childPassName = AZStd::string::format("ReflectionScreenSpace_DownsampleDepthLinear%d", mip);
                childPassDescriptor.m_passName = Name(childPassName);

                RPI::Ptr<Render::ReflectionScreenSpaceDownsampleDepthLinearChildPass> childPass = passSystem->CreatePass<Render::ReflectionScreenSpaceDownsampleDepthLinearChildPass>(childPassDescriptor);
                childPass->SetMipLevel(mip);

                AddChild(childPass);
            }
        }

        void ReflectionScreenSpaceDownsampleDepthLinearPass::BuildInternal()
        {
            RemoveChildren();
            m_flags.m_createChildren = true;

            // retrieve DepthLinear attachment
            RPI::PassAttachment* depthLinearImageAttachment = GetInputBinding(0).GetAttachment().get();

            // retrieve DownsampledDepthLinear attachment
            RPI::PassAttachment* downsampledDepthLinearImageAttachment = GetInputOutputBinding(0).GetAttachment().get();
            m_imageSize = downsampledDepthLinearImageAttachment->m_descriptor.m_image.m_size;
            m_mipLevels = aznumeric_cast<uint32_t>(downsampledDepthLinearImageAttachment->m_descriptor.m_image.m_mipLevels);

            // call ParentPass::BuildInternal() first to configure the slots and auto-add the empty bindings,
            // then we will assign attachments to the bindings
            ParentPass::BuildInternal();

            // setup attachment bindings on the child passes
            uint32_t attachmentIndex = 0;
            for (auto& childPass : m_children)
            {
                // the first child pass reads from DepthLinear and writes to mip0 of DownsampledDepthLinear, subsequent
                // passes read from the previous mip level of DownsampledDepthLinear and write to the current mip level
                uint32_t currentMipLevel = attachmentIndex;
                uint32_t previousMipLevel = AZStd::max(0, aznumeric_cast<int32_t>(currentMipLevel) - 1);
                
                RPI::PassAttachmentBinding& inputAttachmentBinding = childPass->GetInputBinding(0);
                RPI::PassAttachment* inputAttachment = nullptr;

                RHI::ImageViewDescriptor inputViewDesc;

                if (currentMipLevel == 0)
                {
                    inputAttachment = depthLinearImageAttachment;
                }
                else
                {
                    inputAttachment = downsampledDepthLinearImageAttachment;
                    inputViewDesc.m_mipSliceMin = static_cast<int16_t>(previousMipLevel);
                    inputViewDesc.m_mipSliceMax = static_cast<int16_t>(previousMipLevel);
                }

                inputAttachmentBinding.m_unifiedScopeDesc.SetAsImage(inputViewDesc);
                inputAttachmentBinding.SetAttachment(inputAttachment);
                
                // downsampled linear depth output (writing to current mip)
                RHI::ImageViewDescriptor outputViewDesc;
                RPI::PassAttachmentBinding& downsampledDepthOutputAttachmentBinding = childPass->GetOutputBinding(0);
                outputViewDesc.m_mipSliceMin = static_cast<int16_t>(currentMipLevel);
                outputViewDesc.m_mipSliceMax = static_cast<int16_t>(currentMipLevel);
                downsampledDepthOutputAttachmentBinding.m_unifiedScopeDesc.SetAsImage(outputViewDesc);
                downsampledDepthOutputAttachmentBinding.SetAttachment(downsampledDepthLinearImageAttachment);
                
                childPass->UpdateConnectedBindings();
                
                attachmentIndex++;
            }
        }
    }   // namespace RPI
}   // namespace AZ
