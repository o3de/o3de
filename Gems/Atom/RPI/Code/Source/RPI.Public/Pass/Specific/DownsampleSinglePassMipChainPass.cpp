/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>
#include <Atom/RPI.Public/Pass/Specific/DownsampleSinglePassMipChainPass.h>

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
            // do nothing.
        }

        void DownsampleSinglePassMipChainPass::BuildInternal()
        {
            GetInputInfo();
            ComputePass::BuildInternal();
        }

        void DownsampleSinglePassMipChainPass::ResetInternal()
        {
            m_indicesAreInitialized = false;
            m_mipsIndex.Reset();
            m_numWorkGroupsIndex.Reset();
            m_workGroupOffsetIndex.Reset();
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

            RHI::ShaderInputNameIndex imageNameIndex("m_imageDestination");
            RHI::ShaderInputNameIndex image6NameIndex("m_imageDestination6");
            static constexpr uint32_t GloballyCoherentMipIndex = 6;
            for (uint32_t mipIndex = 0; mipIndex < GetMin(m_mipLevelCount, SpdMipLevelCountMax); ++mipIndex)
            {
                RHI::ImageViewDescriptor viewDesc;
                viewDesc.m_mipSliceMin = static_cast<uint16_t>(mipIndex);
                viewDesc.m_mipSliceMax = static_cast<uint16_t>(mipIndex);
                Ptr<RHI::ImageView> imageView = RHI::Factory::Get().CreateImageView();
                const RHI::ResultCode result = imageView->Init(*rhiImage, viewDesc);
                if (result != RHI::ResultCode::Success)
                {
                    AZ_Assert(false, "DownsampleSingelPassMipChainPass failed to create RHI::ImageView.");
                    return;
                }
                if (mipIndex == GloballyCoherentMipIndex)
                {
                    srg.SetImageView(image6NameIndex, imageView.get());
                }
                else
                {
                    srg.SetImageView(imageNameIndex, imageView.get(), mipIndex);
                }
                m_imageViews[mipIndex] = imageView;
            }

            ComputePass::CompileResources(context);
        }

        void DownsampleSinglePassMipChainPass::InitializeConstantIndices()
        {
            if (!m_shaderResourceGroup)
            {
                return;
            }
            const ShaderResourceGroup& srg = *m_shaderResourceGroup;

            m_mipsIndex = srg.FindShaderInputConstantIndex(Name("m_mips"));
            m_numWorkGroupsIndex = srg.FindShaderInputConstantIndex(Name("m_numWorkGroups"));
            m_workGroupOffsetIndex = srg.FindShaderInputConstantIndex(Name("m_workGroupOffset"));
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
                m_inputWidth = attachment->m_descriptor.m_image.m_size.m_width;
                m_inputHeight = attachment->m_descriptor.m_image.m_size.m_height;
            }
        }

        void DownsampleSinglePassMipChainPass::SetConstants()
        {
            if (!m_indicesAreInitialized)
            {
                InitializeConstantIndices();
            }

            if (!m_shaderResourceGroup)
            {
                return;
            }
            ShaderResourceGroup& srg = *m_shaderResourceGroup;

            varAU2(dispatchThreadGroupCountXY);
            varAU2(workGroupOffset); // needed if Left and Top are not 0,0
            varAU2(numWorkGroupsAndMips);
            varAU4(rectInfo) = initAU4(0, 0, m_inputWidth, m_inputHeight); // left, top, width, height
            SpdSetup(dispatchThreadGroupCountXY, workGroupOffset, numWorkGroupsAndMips, rectInfo);

            const AZStd::array<AU1, 2> workGroupOffsetArray =
                { workGroupOffset[0], workGroupOffset[1] };
            bool succeeded = true;
            succeeded &= srg.SetConstant(m_numWorkGroupsIndex, numWorkGroupsAndMips[0]);
            succeeded &= srg.SetConstant(m_mipsIndex, m_mipLevelCount);
            succeeded &= srg.SetConstantArray(m_workGroupOffsetIndex, workGroupOffsetArray);
            AZ_Assert(succeeded, "DownsampleSinglePassMipChainPass failed to set constants.");

            m_targetThreadCountWidth = dispatchThreadGroupCountXY[0];
            m_targetThreadCountHeight = dispatchThreadGroupCountXY[1];
        }
    }   // namespace RPI
}   // namespace AZ
