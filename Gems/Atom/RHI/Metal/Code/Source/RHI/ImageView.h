/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/ImageView.h>
#include <Atom/RHI.Reflect/ImageSubresource.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <RHI/MemoryView.h>

namespace AZ
{
    namespace Metal
    {
        class Image;

        class ImageView final
            : public RHI::ImageView
        {
            using Base = RHI::ImageView;
        public:
            AZ_CLASS_ALLOCATOR(ImageView, AZ::ThreadPoolAllocator, 0);
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

        private:
            ImageView() = default;
            void BuildImageSubResourceRange(const RHI::Resource& resourceBase);

            //////////////////////////////////////////////////////////////////////////
            // RHI::ImageView
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::Resource& resourceBase) override;
            RHI::ResultCode InvalidateInternal() override;
            void ShutdownInternal() override;
            //////////////////////////////////////////////////////////////////////////

            //Create a separate own copy of memoryview to be used for rendering.
            //Internally it may create a new MTLTexture object that reinterprets the original MTLTexture object from Image
            MemoryView m_memoryView;
            
            MTLPixelFormat m_format;
            RHI::ImageSubresourceRange m_imageSubresourceRange;
        };
    }
}
