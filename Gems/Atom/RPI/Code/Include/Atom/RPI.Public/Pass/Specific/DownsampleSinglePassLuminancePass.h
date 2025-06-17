/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RHI/Image.h>
#include <Atom/RPI.Reflect/Pass/PassDescriptor.h>

namespace AZ::RPI
{
    //! This pass takes a texture without auxiliary mip slices as input
    //! a texture with mip slices as output.
    //! It then recursively downsamples that mip to lower mip levels using single dispatch of compute shader.
    class ATOM_RPI_PUBLIC_API DownsampleSinglePassLuminancePass final
        : public ComputePass
    {
        AZ_RPI_PASS(DownsampleSinglePassMipChainPass);

    public:
        AZ_RTTI(DownsampleSinglePassLuminancePass, "{6842F4D2-D884-4E2A-B48B-E9240BCB8F45}", ComputePass);
        AZ_CLASS_ALLOCATOR(DownsampleSinglePassLuminancePass, SystemAllocator);

        static Ptr<DownsampleSinglePassLuminancePass> Create(const PassDescriptor& descriptor);
        virtual ~DownsampleSinglePassLuminancePass() = default;

    private:
        struct SpdGlobalAtomicBuffer
        {
            uint32_t m_counter;
        };

        explicit DownsampleSinglePassLuminancePass(const PassDescriptor& descriptor, AZ::Name supervariant);

        // Pass Behaviour Overrides...
        void BuildInternal() override;
        void ResetInternal() override;
        void FrameBeginInternal(FramePrepareParams params) override;

        // Scope producer functions...
        void CompileResources(const RHI::FrameGraphCompileContext& context) override;

        void BuildGlobalAtomicBuffer();
        void InitializeIndices();
        void GetDestinationInfo();
        void CalculateSpdThreadDimensionAndMips();
        void BuildPassAttachment();

        void SetConstants();

        static constexpr uint32_t SpdMipLevelCountMax = 13;
        static constexpr uint32_t GloballyCoherentMipIndex = 6;
        const Name Mip6Name{"m_mip6"};
        const Name GlobalAtomicName{"m_globalAtomic"};

        // Dimension of the destination mip chain image.
        AZStd::array<uint32_t, 2> m_destinationImageSize = {0, 0};
        uint32_t m_destinationMipLevelCount = 0;

        // Number of mip levels for SPD computation
        // which can be slightly greater than m_destinationMipLevelCount for
        // computation of non power of 2 width.
        uint32_t m_spdMipLevelCount = 0;

        bool m_indicesAreInitialized = false;
        uint32_t m_targetThreadCountWidth = 1;
        uint32_t m_targetThreadCountHeight = 1;
        RHI::ShaderInputConstantIndex m_spdMipLevelCountIndex;
        RHI::ShaderInputConstantIndex m_destinationMipLevelCountIndex;
        RHI::ShaderInputConstantIndex m_numWorkGroupsIndex;
        RHI::ShaderInputConstantIndex m_imageSizeIndex;
        RHI::ShaderInputImageIndex m_imageDestinationIndex;
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
