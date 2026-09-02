/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Pass/GpuDriven/HiZGeneratePass.h>

#include <Atom/RHI/FrameGraphBuilder.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI.Reflect/ImageViewDescriptor.h>

#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Public/Pass/PassFactory.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>

#include <Atom/RPI.Reflect/Pass/ComputePassData.h>
#include <Atom/RPI.Reflect/Pass/DownsampleMipChainPassData.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    namespace RPI
    {
        Ptr<HiZGeneratePass> HiZGeneratePass::Create(const PassDescriptor& descriptor)
        {
            Ptr<HiZGeneratePass> pass = aznew HiZGeneratePass(descriptor);
            return pass;
        }

        HiZGeneratePass::HiZGeneratePass(const PassDescriptor& descriptor)
            : ParentPass(descriptor)
        {
            const auto* passData = PassUtils::GetPassData<DownsampleMipChainPassData>(descriptor);
            if (!passData)
            {
                AZ_Error("PassSystem", false,
                    "[HiZGeneratePass '%s']: No valid DownsampleMipChainPassData!",
                    GetPathName().GetCStr());
                return;
            }

            m_shaderReference = passData->m_shaderReference;
            ShaderReloadNotificationBus::Handler::BusConnect(m_shaderReference.m_assetId);
        }

        HiZGeneratePass::~HiZGeneratePass()
        {
            ShaderReloadNotificationBus::Handler::BusDisconnect();
        }

        void HiZGeneratePass::ResetInternal()
        {
            RemoveChildren();
        }

        void HiZGeneratePass::GetAttachmentInfo()
        {
            AZ_Assert(GetInputCount() > 0, "[HiZGeneratePass '%s']: must have a depth input", GetPathName().GetCStr());
            AZ_Assert(GetInputOutputCount() > 0, "[HiZGeneratePass '%s']: must have a Hi-Z output", GetPathName().GetCStr());

            PassAttachment* depthAttachment = GetInputBinding(0).GetAttachment().get();
            PassAttachment* hiZAttachment = GetInputOutputBinding(0).GetAttachment().get();

            if (depthAttachment && hiZAttachment)
            {
                uint32_t newDepthWidth = depthAttachment->m_descriptor.m_image.m_size.m_width;
                uint32_t newDepthHeight = depthAttachment->m_descriptor.m_image.m_size.m_height;
                uint16_t newMipLevels = hiZAttachment->m_descriptor.m_image.m_mipLevels;

                m_needToRebuildChildren = m_needToRebuildChildren || (m_hiZMipLevels != newMipLevels);
                m_needToUpdateChildren |= (m_depthWidth != newDepthWidth);
                m_needToUpdateChildren |= (m_depthHeight != newDepthHeight);
                m_needToUpdateChildren |= m_needToRebuildChildren;

                m_depthWidth = newDepthWidth;
                m_depthHeight = newDepthHeight;
                m_hiZMipLevels = newMipLevels;
            }
        }

        void HiZGeneratePass::BuildChildPasses()
        {
            RemoveChildren();

            if (m_hiZMipLevels == 0)
            {
                m_needToRebuildChildren = false;
                return;
            }

            PassSystemInterface* passSystem = PassSystemInterface::Get();

            PassDescriptor childDesc;
            childDesc.m_passData = AZStd::make_shared<ComputePassData>();
            auto* passData = static_cast<ComputePassData*>(childDesc.m_passData.get());
            passData->m_shaderReference = m_shaderReference;

            PassAttachmentBinding& depthBinding = GetInputBinding(0);
            PassAttachmentBinding& hiZBinding = GetInputOutputBinding(0);
            const Ptr<PassAttachment>& hiZAttachment = hiZBinding.GetAttachment();

            for (uint16_t mip = 0; mip < m_hiZMipLevels; ++mip)
            {
                AZStd::string childName = AZStd::string::format("HiZMip%d", mip);
                childDesc.m_passName = Name(childName);

                Ptr<ComputePass> childPass = passSystem->CreatePass<ComputePass>(childDesc);

                // Input binding: for mip 0 read from the depth buffer; for mip N read Hi-Z mip N-1
                {
                    PassAttachmentBinding inBinding;
                    inBinding.m_name = "Input";
                    inBinding.m_slotType = PassSlotType::Input;
                    inBinding.m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::Shader;

                    RHI::ImageViewDescriptor inViewDesc;

                    if (mip == 0)
                    {
                        inBinding.m_connectedBinding = &depthBinding;
                        inBinding.SetAttachment(depthBinding.GetAttachment());
                        inViewDesc.m_aspectFlags = RHI::ImageAspectFlags::Depth;
                    }
                    else
                    {
                        inBinding.m_connectedBinding = &hiZBinding;
                        inBinding.SetAttachment(hiZAttachment);
                        inViewDesc.m_mipSliceMin = mip - 1;
                        inViewDesc.m_mipSliceMax = mip - 1;
                    }

                    inBinding.m_unifiedScopeDesc.SetAsImage(inViewDesc);
                    childPass->AddAttachmentBinding(inBinding);
                }

                // Output binding: write to Hi-Z mip N
                {
                    PassAttachmentBinding outBinding;
                    outBinding.m_name = "Output";
                    outBinding.m_slotType = PassSlotType::InputOutput;
                    outBinding.m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::Shader;
                    outBinding.m_connectedBinding = &hiZBinding;

                    RHI::ImageViewDescriptor outViewDesc;
                    outViewDesc.m_mipSliceMin = mip;
                    outViewDesc.m_mipSliceMax = mip;
                    outBinding.m_unifiedScopeDesc.SetAsImage(outViewDesc);
                    outBinding.SetAttachment(hiZAttachment);

                    childPass->AddAttachmentBinding(outBinding);
                }

                AddChild(childPass);
            }

            m_needToRebuildChildren = false;
        }

        void HiZGeneratePass::UpdateChildren()
        {
            // Mip 0 of the Hi-Z is half the depth resolution, each subsequent mip halves again
            uint32_t targetWidth = (m_depthWidth + 1) / 2;
            uint32_t targetHeight = (m_depthHeight + 1) / 2;

            for (const Ptr<Pass>& child : m_children)
            {
                auto* computeChild = static_cast<ComputePass*>(child.get());
                computeChild->SetTargetThreadCounts(targetWidth, targetHeight, 1);

                targetWidth = AZStd::max(1u, (targetWidth + 1) / 2);
                targetHeight = AZStd::max(1u, (targetHeight + 1) / 2);
            }

            m_needToUpdateChildren = false;
        }

        Data::Instance<AttachmentImage> HiZGeneratePass::GetLastCompletedPyramid() const
        {
            if (!m_isPersistent)
            {
                return nullptr;
            }
            return m_persistentPyramidImages[(m_persistentPyramidIndex + 1) % PersistentPyramidCount];
        }

        void HiZGeneratePass::SetupPersistentPyramid()
        {
            static const Name persistentAttachmentName("HiZPyramidPersistent");

            Ptr<PassAttachment> hiZAttachment;
            for (auto& attachment : m_ownedAttachments)
            {
                if (attachment->m_name == persistentAttachmentName)
                {
                    hiZAttachment = attachment;
                    break;
                }
            }

            if (!hiZAttachment)
            {
                m_isPersistent = false;
                return;
            }

            m_isPersistent = true;

            // Sized via the attachment's own SizeSource/GenerateFullMipChain (set by the .pass
            // template) exactly like the stock transient HiZPyramid -- only the lifetime differs.
            hiZAttachment->m_lifetime = RHI::AttachmentLifetimeType::Imported;

            Data::Instance<AttachmentImagePool> pool = ImageSystemInterface::Get()->GetSystemAttachmentPool();
            for (uint32_t i = 0; i < PersistentPyramidCount; ++i)
            {
                AZStd::string imageName = AZStd::string::format("%s.HiZPyramidPersistent_%u", GetPathName().GetCStr(), i);
                m_persistentPyramidImages[i] =
                    AttachmentImage::Create(*pool.get(), hiZAttachment->m_descriptor.m_image, Name(imageName), nullptr, nullptr);
            }

            // A rebuild (e.g. resize) invalidates prior content at the old resolution -- restart the
            // ping-pong and require both slots to be freshly written again before
            // IsPersistentPyramidPopulated() reports true.
            m_persistentPyramidIndex = 0;
            m_framesSincePersistentActivation = 0;
            hiZAttachment->m_importedResource = m_persistentPyramidImages[m_persistentPyramidIndex];

            // Deliberately manual (see the declaration comment): this attachment has no "Connections"
            // entry in the persistent template, so no binding has touched it yet.
            if (PassAttachmentBinding* binding = FindAttachmentBinding(Name("HiZOutput")))
            {
                binding->SetAttachment(hiZAttachment);
            }
        }

        void HiZGeneratePass::SwapPersistentPyramid()
        {
            if (!m_isPersistent)
            {
                return;
            }

            PassAttachmentBinding* binding = FindAttachmentBinding(Name("HiZOutput"));
            Ptr<PassAttachment> hiZAttachment = binding ? binding->GetAttachment() : nullptr;
            if (!hiZAttachment)
            {
                return;
            }

            m_persistentPyramidIndex = (m_persistentPyramidIndex + 1) % PersistentPyramidCount;
            hiZAttachment->m_importedResource = m_persistentPyramidImages[m_persistentPyramidIndex];

            if (m_framesSincePersistentActivation < PersistentPyramidCount)
            {
                ++m_framesSincePersistentActivation;
            }
        }

        void HiZGeneratePass::BuildInternal()
        {
            // Force owned attachment update before reading attachment info.
            // The HiZPyramid's mip count is derived from the DepthInput's size via SizeSource,
            // but Pass::UpdateOwnedAttachments() normally runs after BuildInternal() in Pass::Build().
            // Without this, m_mipLevels would be 0 and no child passes are created during build,
            // causing FrameBeginInternal to attempt structural changes in the wrong lifecycle phase.
            for (auto& attachment : m_ownedAttachments)
            {
                attachment->Update();
            }

            // Phase 7: no-op unless this instance owns a "HiZPyramidPersistent" attachment (see
            // HiZGeneratePersistentTemplate). Must run before GetAttachmentInfo() below, which reads
            // the "HiZOutput" binding's attachment that this call is responsible for setting in that
            // case (the stock template gets it for free via the JSON "Connections" array instead).
            SetupPersistentPyramid();

            GetAttachmentInfo();
            BuildChildPasses();
            UpdateChildren();
            ParentPass::BuildInternal();
        }

        void HiZGeneratePass::FrameBeginInternal(FramePrepareParams params)
        {
            GetAttachmentInfo();

            if (m_needToRebuildChildren)
            {
                // Structural changes (AddChild/RemoveChildren) must not happen during FrameBegin.
                // Queue a proper rebuild through the PassSystem so children are created during
                // the build phase with correct lifecycle transitions.
                QueueForBuildAndInitialization();
                return;
            }

            if (m_needToUpdateChildren)
            {
                UpdateChildren();
            }

            // Ping-pong BEFORE this frame's mip-chain children dispatch (ParentPass::FrameBeginInternal
            // below is what schedules them), so they write into the slot that just became "current".
            SwapPersistentPyramid();

            ParentPass::FrameBeginInternal(params);
        }

        void HiZGeneratePass::OnShaderReinitialized([[maybe_unused]] const Shader& shader)
        {
            m_needToUpdateChildren = true;
        }

        void HiZGeneratePass::OnShaderAssetReinitialized([[maybe_unused]] const Data::Asset<ShaderAsset>& shaderAsset)
        {
            m_needToUpdateChildren = true;
        }

        void HiZGeneratePass::OnShaderVariantReinitialized([[maybe_unused]] const ShaderVariant& shaderVariant)
        {
            m_needToUpdateChildren = true;
        }

    } // namespace RPI
} // namespace AZ
