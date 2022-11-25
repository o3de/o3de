/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Reflect/Pass/PassDescriptor.h>

namespace AZ::RPI
{
    //! This pass takes a mip mapped texture as input where the most detailed mip is already written to.
    //! It then recursively downsamples that mip to lower mip levels using single dispatch of compute shader.
    class DownsampleSinglePassMipChainPass final
        : public ComputePass
    {
        AZ_RPI_PASS(DownsampleSinglePassMipChainPass);

    public:
        AZ_RTTI(DownsampleSinglePassMipChainPass, "{653D5F1C-6070-4DDF-9F0A-2AF831F3C3AA}", ComputePass);
        AZ_CLASS_ALLOCATOR(DownsampleSinglePassMipChainPass, SystemAllocator, 0);

        static Ptr<DownsampleSinglePassMipChainPass> Create(const PassDescriptor& descriptor);
        virtual ~DownsampleSinglePassMipChainPass() = default;

    private:
        struct SpdGlobalAtomicBuffer
        {
            uint32_t m_counter;
        };

        explicit DownsampleSinglePassMipChainPass(const PassDescriptor& descriptor);

        // Pass Behaviour Overrides...
        void BuildInternal() override;
        void ResetInternal() override;
        void FrameBeginInternal(FramePrepareParams params) override;

        // Scope producer functions...
        void CompileResources(const RHI::FrameGraphCompileContext& context) override;

        void BuildGlobalAtomicBuffer();
        void InitializeIndices();
        void GetInputInfo();
        void CalculateBaseSpdImageSize();
        void BuildPassAttachment();

        void SetConstants();

        static constexpr uint32_t SpdMipLevelCountMax = 13;
        static constexpr uint32_t GloballyCoherentMipIndex = 6;
        const Name Mip6Name{"m_mip6"};
        const Name GlobalAtomicName{"m_globalAtomic"};

        // Height and width of the input mip chain texture
        AZStd::array<uint32_t, 2> m_inputImageSize = {0, 0};

        // Base height and width of SPD, which are of power of 2.
        AZStd::array<uint32_t, 2> m_baseSpdImageSize = {0, 0};

        // Number of mip levels in the input mip chain texture
        uint32_t m_mipLevelCount = 0;

        bool m_indicesAreInitialized = false;
        uint32_t m_targetThreadCountWidth = 1;
        uint32_t m_targetThreadCountHeight = 1;
        RHI::ShaderInputConstantIndex m_mipsIndex;
        RHI::ShaderInputConstantIndex m_numWorkGroupsIndex;
        RHI::ShaderInputConstantIndex m_workGroupOffsetIndex;
        RHI::ShaderInputConstantIndex m_imageSizeIndex;
        RHI::ShaderInputImageIndex m_inputOutputImageIndex;
        RHI::ShaderInputImageIndex m_mip6ImageIndex;
        RHI::ShaderInputBufferIndex m_globalAtomicIndex;

        // Attachment for transient image and its image descriptor.
        Ptr<PassAttachment> m_mip6PassAttachment;
        RHI::ImageDescriptor m_mip6ImageDescriptor;

        // Attachment for transient buffer.
        Ptr<PassAttachment> m_counterPassAttachment;

        // Retainer of image views for each mip level of in/out image.
        AZStd::array<Ptr<RHI::ImageView>, SpdMipLevelCountMax> m_imageViews;

        Data::Instance<Buffer> m_globalAtomicBuffer;
    };
}   // namespace AZ::RPI
