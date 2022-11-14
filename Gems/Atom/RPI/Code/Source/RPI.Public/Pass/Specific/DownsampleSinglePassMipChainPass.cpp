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

// For "A_CPU" macro, refer ffx_a.h and below:
// https://github.com/GPUOpen-Effects/FidelityFX-SPD/blob/c52944f547884774a1b33066f740e6bf89f927f5/sample/src/DX12/SPDIntegration.hlsl
#define A_CPU
#ifdef _MSC_VER
#pragma warning(disable: 4505) // unreferenced local function has been removed.
#endif
#include <ffx_a.h>
#include <ffx_spd.h>

namespace AZ
{
    namespace RPI
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

            if (m_globalAtomicBuffer)
            {
                // Clear Global Atomic.
                auto* buffer = static_cast<SpdGlobalAtomicBuffer*>(m_globalAtomicBuffer->Map(sizeof(SpdGlobalAtomicBuffer), 0));
                {
                    buffer->m_counter = 0;
                }
                m_globalAtomicBuffer->Unmap();
            }
            else
            {
                AZ_Assert(false, "DownsampleSingelPassMipChainPass Global Atomic Buffer has not been created.");
            }

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
            for (uint32_t mipIndex = 0; mipIndex < GetMin(m_mipLevelCount, SpdMipLevelCountMax); ++mipIndex)
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
            RPI::CommonBufferDescriptor descriptor;
            descriptor.m_poolType = RPI::CommonBufferPoolType::ReadWrite;
            descriptor.m_bufferName = "DownsampleSinglePassMipChainPass GlobalAtomic";
            descriptor.m_elementSize = sizeof(SpdGlobalAtomicBuffer);
            descriptor.m_byteCount = sizeof(SpdGlobalAtomicBuffer);
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
            AZ_Assert(
                GetInputOutputCount() > 0, "[DownsampleSinglePassMipChainPass '%s']: must have an input/output", GetPathName().GetCStr());
            PassAttachment* attachment = GetInputOutputBinding(0).GetAttachment().get();

            if (attachment)
            {
                m_mipLevelCount = attachment->m_descriptor.m_image.m_mipLevels;
                m_inputImageSize[0] = attachment->m_descriptor.m_image.m_size.m_width;
                m_inputImageSize[1] = attachment->m_descriptor.m_image.m_size.m_height;
            }
        }

        void DownsampleSinglePassMipChainPass::CalculateBaseSpdImageSize()
        {
            const auto adjust = [&] (uint32_t value) -> uint32_t
            {
                const auto logValue = static_cast<uint32_t>(floorf(log2f(value * 1.f)));
                if (static_cast<uint32_t>(1 << logValue) == value)
                {
                    return value;
                }
                else
                {
                    return static_cast<uint32_t>(1 << (logValue + 1));
                }
            };
            m_baseSpdImageSize[0] = adjust(m_inputImageSize[0]);
            m_baseSpdImageSize[1] = adjust(m_inputImageSize[1]);
        }

        void DownsampleSinglePassMipChainPass::BuildPassAttachment()
        {
            // Build "Mip6" image attachment.
            {
                const auto width = GetMax<uint32_t>(1, m_baseSpdImageSize[0] >> GloballyCoherentMipIndex);
                const auto height = GetMax<uint32_t>(1, m_baseSpdImageSize[1] >> GloballyCoherentMipIndex);
                m_mip6ImageDescriptor =
                    RHI::ImageDescriptor::Create2D(
                        RHI::ImageBindFlags::ShaderReadWrite,
                        width, 
                        height,
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
            ShaderResourceGroup& srg = *m_shaderResourceGroup;

            varAU2(dispatchThreadGroupCountXY);
            varAU2(workGroupOffset); // needed if Left and Top are not 0,0
            varAU2(numWorkGroupsAndMips);
            varAU4(rectInfo) = initAU4(
                0, // left
                0, // top
                m_baseSpdImageSize[0], // width
                m_baseSpdImageSize[1]); // height
            SpdSetup(dispatchThreadGroupCountXY, workGroupOffset, numWorkGroupsAndMips, rectInfo);

            const AZStd::array<AU1, 2> workGroupOffsetArray =
                { workGroupOffset[0], workGroupOffset[1] };
            bool succeeded = true;
            succeeded &= srg.SetConstant(m_numWorkGroupsIndex, numWorkGroupsAndMips[0]);
            succeeded &= srg.SetConstant(m_mipsIndex, m_mipLevelCount);
            succeeded &= srg.SetConstantArray(m_workGroupOffsetIndex, workGroupOffsetArray);
            succeeded &= srg.SetConstantArray(m_imageSizeIndex, m_inputImageSize);
            AZ_Assert(succeeded, "DownsampleSinglePassMipChainPass failed to set constants.");

            m_targetThreadCountWidth = dispatchThreadGroupCountXY[0];
            m_targetThreadCountHeight = dispatchThreadGroupCountXY[1];
        }
    }   // namespace RPI
}   // namespace AZ
