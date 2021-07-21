/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/Descriptor.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI/ImageView.h>
#include <AzCore/Memory/PoolAllocator.h>

namespace AZ
{
    namespace DX12
    {
        class Image;

        class ImageView final
            : public RHI::ImageView
        {
            using Base = RHI::ImageView;
        public:
            AZ_CLASS_ALLOCATOR(ImageView, AZ::ThreadPoolAllocator, 0);
            AZ_RTTI(ImageView, "{FEC44057-C031-4454-9326-94758C4F729A}", Base);

            static RHI::Ptr<ImageView> Create();

            /// Overrides the RHI methods to return DX12 versions of Image.
            const Image& GetImage() const;

            /// Gets the specific image view format.
            DXGI_FORMAT GetFormat() const;

            /// Returns the DX12 resource associated with the view.
            ID3D12Resource* GetMemory() const;

            /// Returns the descriptor handles for each type of access.
            DescriptorHandle GetReadDescriptor() const;
            DescriptorHandle GetReadWriteDescriptor() const;
            DescriptorHandle GetClearDescriptor() const;
            DescriptorHandle GetColorDescriptor() const;
            DescriptorHandle GetDepthStencilDescriptor(RHI::ScopeAttachmentAccess access) const;

        private:
            ImageView() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::ImageView
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::Resource& resourceBase) override;
            RHI::ResultCode InvalidateInternal() override;
            void ShutdownInternal() override;
            //////////////////////////////////////////////////////////////////////////

            ID3D12Resource* m_memory = nullptr;
            DXGI_FORMAT m_format = DXGI_FORMAT_UNKNOWN;
            DescriptorHandle m_readDescriptor;
            DescriptorHandle m_readWriteDescriptor;
            DescriptorHandle m_clearDescriptor;
            DescriptorHandle m_colorDescriptor;
            DescriptorHandle m_depthStencilDescriptor;
            DescriptorHandle m_depthStencilReadDescriptor;
        };
    }
}
