/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHIUtils.h>
#include <Atom/RPI.Public/Pass/Specific/DownsampleSinglePassLuminancePass.h>
#include <Atom/RPI.Reflect/Buffer/BufferAssetCreator.h>

namespace AZ::RPI
{
    Ptr<DownsampleSinglePassLuminancePass> DownsampleSinglePassLuminancePass::Create(const PassDescriptor& descriptor)
    {
        // Check capability of wave operation
        bool isWaveSupported = RHI::GetRHIDevice()->GetFeatures().m_waveOperation;
        const char* supervariantName = isWaveSupported ? "" : AZ::RPI::NoWaveSupervariantName;
        Ptr<DownsampleSinglePassLuminancePass> pass = aznew DownsampleSinglePassLuminancePass(descriptor, AZ::Name(supervariantName));
        return pass;
    }

    DownsampleSinglePassLuminancePass::DownsampleSinglePassLuminancePass(const PassDescriptor& descriptor, AZ::Name supervariant)
        : ComputePass(descriptor, supervariant)
    {
        BuildGlobalAtomicBuffer();
    }

    void DownsampleSinglePassLuminancePass::BuildInternal()
    {
        GetDestinationInfo();
        CalculateSpdThreadDimensionAndMips();
        BuildPassAttachment();
        ComputePass::BuildInternal();
    }

    void DownsampleSinglePassLuminancePass::ResetInternal()
    {
        m_indicesAreInitialized = false;
        m_spdMipLevelCountIndex.Reset();
        m_destinationMipLevelCountIndex.Reset();
        m_numWorkGroupsIndex.Reset();
        m_imageSizeIndex.Reset();
        m_imageDestinationIndex.Reset();
        m_mip6ImageIndex.Reset();
        m_globalAtomicIndex.Reset();
        ComputePass::ResetInternal();
    }

    void DownsampleSinglePassLuminancePass::FrameBeginInternal(FramePrepareParams params)
    {
        SetConstants();
        ComputePass::FrameBeginInternal(params);
    }

    void DownsampleSinglePassLuminancePass::CompileResources(const RHI::FrameGraphCompileContext& context)
    {
        if (!m_shaderResourceGroup)
        {
            return;
        }
        ShaderResourceGroup& srg = *m_shaderResourceGroup;

        static constexpr uint32_t ThreadGroupSizeX = 256;
        static constexpr uint32_t ArraySliceCount = 1;
        SetTargetThreadCounts(m_targetThreadCountWidth * ThreadGroupSizeX,
            m_targetThreadCountHeight,
            ArraySliceCount);

        // Input/Output Mip Slices
        PassAttachmentBinding& outBinding = GetOutputBinding(0);
        PassAttachment* attachment = outBinding.GetAttachment().get();
        if (!attachment)
        {
            return;
        }
        const uint32_t mipLevelCount = attachment->m_descriptor.m_image.m_mipLevels;
        RHI::AttachmentId attachmentId = attachment->GetAttachmentId();
        const RHI::Image* rhiImage = context.GetImage(attachmentId);
        if (!rhiImage)
        {
            return;
        }

        RHI::ImageViewDescriptor imageViewDescriptor;
        for (uint32_t mipIndex = 0; mipIndex < GetMin(mipLevelCount, SpdMipLevelCountMax); ++mipIndex)
        {
            imageViewDescriptor.m_mipSliceMin = static_cast<uint16_t>(mipIndex);
            imageViewDescriptor.m_mipSliceMax = static_cast<uint16_t>(mipIndex);
            Ptr<RHI::ImageView> imageView = const_cast<RHI::Image*>(rhiImage)->BuildImageView(imageViewDescriptor);
            srg.SetImageView(m_imageDestinationIndex, imageView.get(), mipIndex);
            m_imageViews[mipIndex] = imageView;
        }

        // Set Globally coherent image view.
        const RHI::ImageView* mip6ImageView = context.GetImageView(m_mip6PassAttachment->GetAttachmentId());
        srg.SetImageView(m_mip6ImageIndex, mip6ImageView);

        // Set Global Atomic buffer.
        srg.SetBuffer(m_globalAtomicIndex, m_globalAtomicBuffer);

        ComputePass::CompileResources(context);
    }

    void DownsampleSinglePassLuminancePass::BuildGlobalAtomicBuffer()
    {
        const SpdGlobalAtomicBuffer initialData = {0};

        RPI::CommonBufferDescriptor descriptor;
        descriptor.m_poolType = RPI::CommonBufferPoolType::ReadWrite;
        descriptor.m_bufferName = "DownsampleSinglePassMipChainPass GlobalAtomic";
        descriptor.m_elementSize = sizeof(SpdGlobalAtomicBuffer);
        descriptor.m_byteCount = sizeof(SpdGlobalAtomicBuffer);
        descriptor.m_bufferData = reinterpret_cast<const void*>(&initialData);
        m_globalAtomicBuffer = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(descriptor);
        AZ_Assert(m_globalAtomicBuffer, "DownsampleSinglePassMipChainPass Building Global Atomic Buffer failed.")
    }

    void DownsampleSinglePassLuminancePass::InitializeIndices()
    {
        if (!m_shaderResourceGroup)
        {
            return;
        }
        const ShaderResourceGroup& srg = *m_shaderResourceGroup;

        m_spdMipLevelCountIndex = srg.FindShaderInputConstantIndex(Name("m_spdMipLevelCount"));
        m_destinationMipLevelCountIndex = srg.FindShaderInputConstantIndex(Name("m_destinationMipLevelCount"));
        m_numWorkGroupsIndex = srg.FindShaderInputConstantIndex(Name("m_numWorkGroups"));
        m_imageSizeIndex = srg.FindShaderInputConstantIndex(Name("m_imageSize"));
        m_imageDestinationIndex = srg.FindShaderInputImageIndex(Name("m_imageDestination"));
        m_mip6ImageIndex = srg.FindShaderInputImageIndex(Mip6Name);
        m_globalAtomicIndex = srg.FindShaderInputBufferIndex(GlobalAtomicName);
        m_indicesAreInitialized = true;
    }

    void DownsampleSinglePassLuminancePass::GetDestinationInfo()
    {
        // Get the input attachment for this pass (at binding 0)
        AZ_Error(
            "DownsampleSinglePassMipChainPass",
            GetInputCount() > 0,
            "[DownsampleSinglePassMipChainPass '%s']: must have an input",
            GetPathName().GetCStr());
        PassAttachment* attachment = GetInputBinding(0).GetAttachment().get();

        if (attachment)
        {
            m_destinationImageSize[0] = attachment->m_descriptor.m_image.m_size.m_width;
            m_destinationImageSize[1] = attachment->m_descriptor.m_image.m_size.m_height;

            // m_mipLevels of the attachment has not been initialized yet, so it is calculated below:
            const uint32_t maxDimension = GetMax(m_destinationImageSize[0], m_destinationImageSize[1]);
            m_destinationMipLevelCount = static_cast<uint32_t>(floorf(log2f(maxDimension * 1.f)));
        }
    }

    void DownsampleSinglePassLuminancePass::CalculateSpdThreadDimensionAndMips()
    {
        // Each SPD thread group computes for sub-region of size 64x64 in mip level 0 slice
        // where 64 == (1 << GloballyCoherentMipIndex).
        static const uint32_t groupImageWidth = 1 << GloballyCoherentMipIndex;
        m_targetThreadCountWidth = (m_destinationImageSize[0] + groupImageWidth - 1) / groupImageWidth;
        m_targetThreadCountHeight = (m_destinationImageSize[1] + groupImageWidth - 1) / groupImageWidth;

        const uint32_t maxDimension = GetMax(m_destinationImageSize[0], m_destinationImageSize[1]);
        m_spdMipLevelCount = m_destinationMipLevelCount;
        if (static_cast<uint32_t>(1 << m_spdMipLevelCount) != maxDimension)
        {
            ++m_spdMipLevelCount;
        }
    }

    void DownsampleSinglePassLuminancePass::BuildPassAttachment()
    {
        // Build "Mip6" image attachment.
        {
            // SPD stores each mip level value into groupshared float arrays except mip 6,
            // and do into this "Mip6" image only for mip 6. So the precision of the image
            // should be same as float variable (32 bit float).
            m_mip6ImageDescriptor =
                RHI::ImageDescriptor::Create2D(
                    RHI::ImageBindFlags::ShaderReadWrite,
                    m_targetThreadCountWidth,
                    m_targetThreadCountHeight,
                    RHI::Format::R32G32B32A32_FLOAT);

            m_mip6PassAttachment = aznew PassAttachment();
            m_mip6PassAttachment->m_name = "Mip6";
            const Name attachmentPath(AZStd::string::format(
                "%s.%s",
                GetPathName().GetCStr(),
                m_mip6PassAttachment->m_name.GetCStr()));
            m_mip6PassAttachment->m_path = attachmentPath;
            m_mip6PassAttachment->m_lifetime = RHI::AttachmentLifetimeType::Transient;
            m_mip6PassAttachment->m_descriptor = m_mip6ImageDescriptor;
            m_ownedAttachments.push_back(m_mip6PassAttachment);

            PassAttachmentBinding binding;
            binding.m_name = m_mip6PassAttachment->m_name;
            binding.m_slotType = PassSlotType::InputOutput;
            binding.m_shaderInputName = Mip6Name;
            binding.m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::Shader;
            binding.SetAttachment(m_mip6PassAttachment);
            AddAttachmentBinding(binding);
        }

        // Build "GlobalAtomic" buffer attachment.
        {
            auto bufferDescriptor = RHI::BufferDescriptor(RHI::BufferBindFlags::ShaderReadWrite, 4);
            bufferDescriptor.m_alignment = 4;

            m_counterPassAttachment = aznew PassAttachment();
            m_counterPassAttachment->m_name = "GlobalAtomic";
            const Name attachmentPath(AZStd::string::format(
                    "%s.%s",
                    GetPathName().GetCStr(),
                    m_counterPassAttachment->m_name.GetCStr()));
            m_counterPassAttachment->m_path = attachmentPath;
            m_counterPassAttachment->m_lifetime = RHI::AttachmentLifetimeType::Imported;
            m_counterPassAttachment->m_descriptor = bufferDescriptor;
            m_counterPassAttachment->m_importedResource = m_globalAtomicBuffer;
            m_ownedAttachments.push_back(m_counterPassAttachment);

            PassAttachmentBinding binding;
            binding.m_name = m_counterPassAttachment->m_name;
            binding.m_slotType = PassSlotType::InputOutput;
            binding.m_shaderInputName = GlobalAtomicName;
            binding.m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::Shader;
            binding.SetAttachment(m_counterPassAttachment);
            AddAttachmentBinding(binding);
        }
    }

    void DownsampleSinglePassLuminancePass::SetConstants()
    {
        if (!m_indicesAreInitialized)
        {
            InitializeIndices();
        }

        if (!m_shaderResourceGroup)
        {
            return;
        }

        // For the setting up of the parameter for SPD shader, refer to:
        // https://github.com/GPUOpen-Effects/FidelityFX-SPD/blob/c52944f547884774a1b33066f740e6bf89f927f5/ffx-spd/ffx_spd.h#L327

        [[maybe_unused]] bool succeeded = true;
        ShaderResourceGroup& srg = *m_shaderResourceGroup;
        succeeded &= srg.SetConstant(
            m_numWorkGroupsIndex,
            m_targetThreadCountWidth * m_targetThreadCountHeight);
        succeeded &= srg.SetConstant(m_spdMipLevelCountIndex, m_spdMipLevelCount);
        succeeded &= srg.SetConstant(m_destinationMipLevelCountIndex, m_destinationMipLevelCount);
        succeeded &= srg.SetConstantArray(m_imageSizeIndex, m_destinationImageSize);
        AZ_Assert(succeeded, "DownsampleSinglePassMipChainPass failed to set constants.");
    }
}   // namespace AZ::RPI
