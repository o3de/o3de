/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceImageView.h>
#include <Atom/RHI.Reflect/ImageSubresource.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <RHI/MemoryView.h>

namespace AZ
{
    namespace Metal
    {
        class Image;

        class ImageView final
            : public RHI::DeviceImageView
        {
            using Base = RHI::DeviceImageView;
        public:
            AZ_CLASS_ALLOCATOR(ImageView, AZ::ThreadPoolAllocator);
            AZ_RTTI(ImageView, "{8D509777-8BF1-4652-B0B1-539C7225DAE9}", Base);

            static RHI::Ptr<ImageView> Create();

            const Image& GetImage() const;
            const RHI::ImageSubresourceRange& GetImageSubresourceRange() const;
            
            const MemoryView& GetMemoryView() const;
            MemoryView& GetMemoryView();
            
            inline MTLPixelFormat GetSpecificFormat() const
            {
                return m_format;
            }

            //! Get the index related to the position of the read and readwrite view within the global Bindless Argument Buffer
            uint32_t GetBindlessReadIndex() const override;
            uint32_t GetBindlessReadWriteIndex() const override;
            
        private:
            ImageView() = default;
            void BuildImageSubResourceRange(const RHI::DeviceResource& resourceBase);

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceImageView
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::DeviceResource& resourceBase) override;
            RHI::ResultCode InvalidateInternal() override;
            void ShutdownInternal() override;
            //////////////////////////////////////////////////////////////////////////

            void ReleaseView();
            void ReleaseBindlessIndices();

            //Create a separate own copy of MemoryView to be used for rendering.
            //Internally it may create a new MTLTexture object that reinterprets the original MTLTexture object from Image
            MemoryView m_memoryView;
            
            MTLPixelFormat m_format;
            RHI::ImageSubresourceRange m_imageSubresourceRange;
            
            //! Index related to the position of the read and readwrite view within the global Bindless Argument Buffer
            uint32_t m_readIndex = InvalidBindlessIndex;
            uint32_t m_readWriteIndex = InvalidBindlessIndex;
        };
    }
}
