/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <math.h>

#include <AtomCore/Instance/InstanceDatabase.h>
#include <AtomCore/std/containers/vector_set.h>

#include <Atom/RHI/FrameGraphBuilder.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>

#include <Atom/RHI.Reflect/ImageViewDescriptor.h>

#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Public/Pass/PassDefines.h>
#include <Atom/RPI.Public/Pass/PassFactory.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/Pass/Specific/DownsampleMipChainPass.h>
#include <Atom/RPI.Public/RenderPipeline.h>

#include <Atom/RPI.Reflect/Pass/ComputePassData.h>
#include <Atom/RPI.Reflect/Pass/PassRequest.h>

namespace AZ
{
    namespace RPI
    {
        Ptr<DownsampleMipChainPass> DownsampleMipChainPass::Create(const PassDescriptor& descriptor)
        {
            Ptr<DownsampleMipChainPass> pass = aznew DownsampleMipChainPass(descriptor);
            return pass;
        }

        DownsampleMipChainPass::DownsampleMipChainPass(const PassDescriptor& descriptor)
            : ParentPass(descriptor)
        {
            // Load DownsampleMipChainPassData
            const DownsampleMipChainPassData* passData = PassUtils::GetPassData<DownsampleMipChainPassData>(descriptor);
            if (passData == nullptr)
            {
                AZ_Error("PassSystem", false, "[DownsampleMipChainPass '%s']: Trying to construct without valid DownsampleMipChainPassData!",
                    GetPathName().GetCStr());
                return;
            }

            m_passData = *passData;
            ShaderReloadNotificationBus::Handler::BusConnect(passData->m_shaderReference.m_assetId);
        }

        DownsampleMipChainPass::~DownsampleMipChainPass()
        {
            ShaderReloadNotificationBus::Handler::BusDisconnect();
        }
    
        void DownsampleMipChainPass::ResetInternal()
        {
            RemoveChildren();
        }

        void DownsampleMipChainPass::GetInputInfo()
        {
            // Get the input/output mip chain attachment for this pass (at binding 0)
            AZ_Assert(GetInputOutputCount() > 0, "[DownsampleMipChainPass '%s']: must have an input/output", GetPathName().GetCStr());
            PassAttachment* attachment = GetInputOutputBinding(0).m_attachment.get();

            if (attachment != nullptr)
            {
                // Check if we need to rebuild children because the number of mips has changed
                m_needToRebuildChildren = m_needToRebuildChildren || (m_mipLevels != attachment->m_descriptor.m_image.m_mipLevels);

                // Check if we need to update children because the image dimensions have changed
                m_needToUpdateChildren |= (m_inputWidth != attachment->m_descriptor.m_image.m_size.m_width);
                m_needToUpdateChildren |= (m_inputHeight != attachment->m_descriptor.m_image.m_size.m_height);
                m_needToUpdateChildren |= m_needToRebuildChildren;

                // Get parameters from the image descriptor
                m_mipLevels = attachment->m_descriptor.m_image.m_mipLevels;
                m_inputWidth = attachment->m_descriptor.m_image.m_size.m_width;
                m_inputHeight = attachment->m_descriptor.m_image.m_size.m_height;
            }
        }

        void DownsampleMipChainPass::BuildChildPasses()
        {
            RemoveChildren();

            PassSystemInterface* passSystem = PassSystemInterface::Get();

            PassDescriptor childDesc;
            childDesc.m_passData = AZStd::make_shared<ComputePassData>();

            ComputePassData* passData = static_cast<ComputePassData*>(childDesc.m_passData.get());
            passData->m_shaderReference = m_passData.m_shaderReference;

            PassAttachmentBinding& inOutBinding = GetInputOutputBinding(0);
            Ptr<PassAttachment>& inOutAttachment = inOutBinding.m_attachment;

            // We are downsampling a mip chain where the first mip is already written to
            // Create passes to write to the other [mipLevels - 1] mips
            for(uint16_t mip = 0; mip < m_mipLevels - 1; ++mip)
            {
                // Generate name based on mip
                AZStd::string childName = AZStd::string::format("DownSample%d", mip);
                childDesc.m_passName = Name(childName);

                // Create Pass
                Ptr<ComputePass> childPass = passSystem->CreatePass<ComputePass>(childDesc);

                // Create input binding for child
                {
                    PassAttachmentBinding inBinding;
                    inBinding.m_name = "Input";
                    inBinding.m_slotType = PassSlotType::Input;
                    inBinding.m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::Shader;
                    inBinding.m_attachment = inOutAttachment;
                    inBinding.m_connectedBinding = &inOutBinding;

                    RHI::ImageViewDescriptor inViewDesc;
                    inViewDesc.m_mipSliceMin = mip;
                    inViewDesc.m_mipSliceMax = mip;
                    inBinding.m_unifiedScopeDesc.SetAsImage(inViewDesc);

                    childPass->AddAttachmentBinding(inBinding);
                }

                // Create output binding for child
                {
                    PassAttachmentBinding outBinding;
                    outBinding.m_name = "Output";
                    outBinding.m_slotType = PassSlotType::InputOutput;
                    outBinding.m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::Shader;
                    outBinding.m_attachment = inOutAttachment;
                    outBinding.m_connectedBinding = &inOutBinding;

                    RHI::ImageViewDescriptor outViewDesc;
                    outViewDesc.m_mipSliceMin = mip + 1;
                    outViewDesc.m_mipSliceMax = mip + 1;
                    outBinding.m_unifiedScopeDesc.SetAsImage(outViewDesc);

                    childPass->AddAttachmentBinding(outBinding);
                }

                AddChild(childPass);
            }

            m_needToRebuildChildren = false;
        }

        void DownsampleMipChainPass::UpdateChildren()
        {
            AZ_Assert(m_children.size() == m_mipLevels - 1,
                "[DownsampleMipChainPass '%s']: number of child passes (%d) does not match number of mips (%d)",
                GetPathName().GetCStr(),
                m_children.size(),
                m_mipLevels - 1);

            uint32_t sourceWidth = m_inputWidth;
            uint32_t sourceHeight = m_inputHeight;

            uint32_t targetWidth = (sourceWidth + 1) / 2;
            uint32_t targetHeight = (sourceHeight + 1) / 2;

            for (const Ptr<Pass>& child : m_children)
            {
                ComputePass* computeChild = static_cast<ComputePass*>(child.get());
                computeChild->SetTargetThreadCounts(targetWidth, targetHeight, 1);

                sourceWidth = targetWidth;
                sourceHeight = targetHeight;

                targetWidth = (sourceWidth + 1) / 2;
                targetHeight = (sourceHeight + 1) / 2;
            }

            m_needToUpdateChildren = false;
        }

        // Pass behavior functions...

        void DownsampleMipChainPass::BuildInternal()
        {
            GetInputInfo();
            BuildChildPasses();
            UpdateChildren();
            ParentPass::BuildInternal();
        }

        void DownsampleMipChainPass::FrameBeginInternal(FramePrepareParams params)
        {
            GetInputInfo();

            if (m_needToRebuildChildren)
            {
                BuildChildPasses();
            }

            if (m_needToUpdateChildren)
            {
                UpdateChildren();
            }

            ParentPass::FrameBeginInternal(params);
        }

        void DownsampleMipChainPass::OnShaderReinitialized([[maybe_unused]] const Shader& shader)
        {
            m_needToUpdateChildren = true;
        }
    
        void DownsampleMipChainPass::OnShaderAssetReinitialized([[maybe_unused]] const Data::Asset<ShaderAsset>& shaderAsset)
        {
            m_needToUpdateChildren = true;
        }
    
        void DownsampleMipChainPass::OnShaderVariantReinitialized([[maybe_unused]] const ShaderVariant& shaderVariant)
        {
            m_needToUpdateChildren = true;
        }
    }   // namespace RPI
}   // namespace AZ
