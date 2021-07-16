/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/ImageView.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <RHI/Image.h>

namespace AZ
{
    namespace Vulkan
    {
        class SwapChain;

        class ImageView final
            : public RHI::ImageView
        {
            using Base = RHI::ImageView;
        public:
            using ResourceType = Image;

            AZ_CLASS_ALLOCATOR(ImageView, AZ::ThreadPoolAllocator, 0);
            AZ_RTTI(ImageView, "0BDF7F84-BBC8-40C7-BC9A-686E22099643", Base);

            static RHI::Ptr<ImageView> Create();
            ~ImageView() = default;

            VkImageView GetNativeImageView() const;
            RHI::Format GetFormat() const;
            const RHI::ImageSubresourceRange& GetImageSubresourceRange() const;
            const VkImageSubresourceRange& GetVkImageSubresourceRange() const;

        private:
            ImageView() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::ImageView
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::Resource& resourceBase) override;
            RHI::ResultCode InvalidateInternal() override;
            void ShutdownInternal() override;
            //////////////////////////////////////////////////////////////////////////

            VkImageViewType GetImageViewType(const Image& image) const;
            void BuildImageSubresourceRange(VkImageViewType imageViewType, VkImageAspectFlags aspectFlags);

            VkImageView m_vkImageView = VK_NULL_HANDLE;
            RHI::Format m_format = RHI::Format::Unknown;
            RHI::ImageSubresourceRange m_imageSubresourceRange;
            VkImageSubresourceRange m_vkImageSubResourceRange;
        };
    }
}
