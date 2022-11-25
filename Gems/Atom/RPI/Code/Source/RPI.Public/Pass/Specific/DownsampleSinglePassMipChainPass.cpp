/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>
#include <Atom/RPI.Public/Pass/Specific/DownsampleSinglePassMipChainPass.h>
#include <Atom/RPI.Reflect/Buffer/BufferAssetCreator.h>

namespace AZ::RPI
{
    Ptr<DownsampleSinglePassMipChainPass> DownsampleSinglePassMipChainPass::Create(const PassDescriptor& descriptor)
    {
        Ptr<DownsampleSinglePassMipChainPass> pass = aznew DownsampleSinglePassMipChainPass(descriptor);
        return pass;
    }

    DownsampleSinglePassMipChainPass::DownsampleSinglePassMipChainPass(const PassDescriptor& descriptor)
        : ComputePass(descriptor)
    {
        BuildGlobalAtomicBuffer();
    }

    void DownsampleSinglePassMipChainPass::BuildInternal()
    {
        GetInputInfo();
        CalculateBaseSpdImageSize();
        BuildPassAttachment();
        ComputePass::BuildInternal();
    }

    void DownsampleSinglePassMipChainPass::ResetInternal()
    {
        m_indicesAreInitialized = false;
        m_mipsIndex.Reset();
        m_numWorkGroupsIndex.Reset();
        m_workGroupOffsetIndex.Reset();
        m_imageSizeIndex.Reset();
        m_inputOutputImageIndex.Reset();
        m_mip6ImageIndex.Reset();
        m_globalAtomicIndex.Reset();
        ComputePass::ResetInternal();
    }

    void DownsampleSinglePassMipChainPass::FrameBeginInternal(FramePrepareParams params)
    {
        SetConstants();
        ComputePass::FrameBeginInternal(params);
    }

    void DownsampleSinglePassMipChainPass::CompileResources(const RHI::FrameGraphCompileContext& context)
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
        PassAttachmentBinding& inOutBinding = GetInputOutputBinding(0);
        PassAttachment* attachment = inOutBinding.GetAttachment().get();
        if (!attachment)
        {
            return;
        }
        RHI::AttachmentId attachmentId = attachment->GetAttachmentId();
        const RHI::Image* rhiImage = context.GetImage(attachmentId);
        if (!rhiImage)
        {
            return;
        }

        RHI::ResultCode result = RHI::ResultCode::Success;
        RHI::ImageViewDescriptor imageViewDescriptor;
        for (uint32_t mipIndex = 0; mipIndex < GetMin(m_inputMipLevelCount, SpdMipLevelCountMax); ++mipIndex)
        {
            imageViewDescriptor.m_mipSliceMin = static_cast<uint16_t>(mipIndex);
            imageViewDescriptor.m_mipSliceMax = static_cast<uint16_t>(mipIndex);
            Ptr<RHI::ImageView> imageView = RHI::Factory::Get().CreateImageView();
            result = imageView->Init(*rhiImage, imageViewDescriptor);
            if (result != RHI::ResultCode::Success)
            {
                AZ_Assert(false, "DownsampleSingelPassMipChainPass failed to create RHI::ImageView.");
                return;
            }
            srg.SetImageView(m_inputOutputImageIndex, imageView.get(), mipIndex);
            m_imageViews[mipIndex] = imageView;
        }

        // Set Globally coherent image view.
        const RHI::ImageView* mip6ImageView = context.GetImageView(m_mip6PassAttachment->GetAttachmentId());
        srg.SetImageView(m_mip6ImageIndex, mip6ImageView);

        // Set Global Atomic buffer.
        srg.SetBuffer(m_globalAtomicIndex, m_globalAtomicBuffer);

        ComputePass::CompileResources(context);
    }

    void DownsampleSinglePassMipChainPass::BuildGlobalAtomicBuffer()
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

    void DownsampleSinglePassMipChainPass::InitializeIndices()
    {
        if (!m_shaderResourceGroup)
        {
            return;
        }
        const ShaderResourceGroup& srg = *m_shaderResourceGroup;

        m_mipsIndex = srg.FindShaderInputConstantIndex(Name("m_mips"));
        m_numWorkGroupsIndex = srg.FindShaderInputConstantIndex(Name("m_numWorkGroups"));
        m_workGroupOffsetIndex = srg.FindShaderInputConstantIndex(Name("m_workGroupOffset"));
        m_imageSizeIndex = srg.FindShaderInputConstantIndex(Name("m_imageSize"));
        m_inputOutputImageIndex = srg.FindShaderInputImageIndex(Name("m_imageDestination"));
        m_mip6ImageIndex = srg.FindShaderInputImageIndex(Mip6Name);
        m_globalAtomicIndex = srg.FindShaderInputBufferIndex(GlobalAtomicName);
        m_indicesAreInitialized = true;
    }

    void DownsampleSinglePassMipChainPass::GetInputInfo()
    {
        // Get the input/output mip chain attachment for this pass (at binding 0)
        AZ_Error(
            "DownsampleSinglePassMipChainPass",
            GetInputOutputCount() > 0,
            "[DownsampleSinglePassMipChainPass '%s']: must have an input/output",
            GetPathName().GetCStr());
        PassAttachment* attachment = GetInputOutputBinding(0).GetAttachment().get();

        if (attachment)
        {
            m_inputMipLevelCount = attachment->m_descriptor.m_image.m_mipLevels;
            m_inputImageSize[0] = attachment->m_descriptor.m_image.m_size.m_width;
            m_inputImageSize[1] = attachment->m_descriptor.m_image.m_size.m_height;
        }
    }

    void DownsampleSinglePassMipChainPass::CalculateBaseSpdImageSize()
    {
        const auto adjust = [&] (uint32_t value, uint32_t& outValue) -> uint32_t
        {
            auto logValue = static_cast<uint32_t>(floorf(log2f(value * 1.f)));
            if (static_cast<uint32_t>(1 << logValue) == value)
            {
                outValue = value;
            }
            else
            {
                logValue += 1;
                outValue = static_cast<uint32_t>(1 << logValue);
            }
            return logValue;

        };
        const uint32_t logValue0 = adjust(m_inputImageSize[0], m_baseSpdImageSize[0]);
        const uint32_t logValue1 = adjust(m_inputImageSize[1], m_baseSpdImageSize[1]);
        m_baseMipLevelCount = GetMax(logValue0, logValue1);
        
        m_targetThreadCountWidth = GetMax<uint32_t>(1, m_baseSpdImageSize[0] >> GloballyCoherentMipIndex);
        m_targetThreadCountHeight = GetMax<uint32_t>(1, m_baseSpdImageSize[1] >> GloballyCoherentMipIndex);
    }

    void DownsampleSinglePassMipChainPass::BuildPassAttachment()
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

            RHI::AttachmentLoadStoreAction action;
            // Components: (min, average, max, weight)
            action.m_clearValue = RHI::ClearValue::CreateVector4Float(FLT_MAX, 0.f, 0.f, 0.f);
            action.m_loadAction = RHI::AttachmentLoadAction::Clear;

            PassAttachmentBinding binding;
            binding.m_name = m_mip6PassAttachment->m_name;
            binding.m_slotType = PassSlotType::InputOutput;
            binding.m_shaderInputName = Mip6Name;
            binding.m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::Shader;
            binding.m_unifiedScopeDesc.m_loadStoreAction = action;
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

    void DownsampleSinglePassMipChainPass::SetConstants()
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

        const AZStd::array<uint32_t, 2> workGroupOffsetArray = {0, 0};
        bool succeeded = true;
        ShaderResourceGroup& srg = *m_shaderResourceGroup;
        succeeded &= srg.SetConstant(
            m_numWorkGroupsIndex,
            m_targetThreadCountWidth * m_targetThreadCountHeight);
        succeeded &= srg.SetConstant(m_mipsIndex, m_baseMipLevelCount);
        succeeded &= srg.SetConstantArray(m_workGroupOffsetIndex, workGroupOffsetArray);
        succeeded &= srg.SetConstantArray(m_imageSizeIndex, m_inputImageSize);
        AZ_Assert(succeeded, "DownsampleSinglePassMipChainPass failed to set constants.");
    }
}   // namespace AZ::RPI
