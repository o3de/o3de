/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Image.h>
#include <Atom/RHI/ImageProperty.h>
#include <Atom/RHI.Reflect/ImageDescriptor.h>
#include <Atom/RHI.Reflect/ImageSubresource.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/Vulkan/ImagePoolDescriptor.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/sort.h>
#include <RHI/MemoryView.h>
#include <RHI/Queue.h>
#include <Atom/RHI/AsyncWorkQueue.h>

namespace AZ
{
    namespace RHI
    {
        class ImageView;
    };
    namespace Vulkan
    {
        class ImagePool;
        class StreamingImagePool;
        class Fence;
        class AliasedHeap;

        class Image final
            : public RHI::Image
        {
            using Base = RHI::Image;
            friend class ImagePool;
            friend class StreamingImagePool;
            friend class AliasedHeap;
            friend class Device;

        public:
            AZ_CLASS_ALLOCATOR(Image, AZ::SystemAllocator, 0);
            AZ_RTTI(Image, "725F56BF-5CCA-4110-91EE-C94E84A35B2C", Base);

            static RHI::Ptr<Image> Create();

            ~Image();
            
            /// It is internally used to handle VkImage as Vulkan::Image class.
            RHI::ResultCode Init(Device& device, VkImage image, const RHI::ImageDescriptor& descriptor);

            void Invalidate();

            const MemoryView* GetMemoryView() const;
            MemoryView* GetMemoryView();

            VkImage GetNativeImage() const;

            bool IsOwnerOfNativeImage() const;

            VkImageAspectFlags GetImageAspectFlags() const;

            size_t GetResidentSizeInBytes() const;
            void SetResidentSizeInBytes(size_t sizeInBytes);

            uint16_t GetStreamedMipLevel() const;
            void SetStreamedMipLevel(uint16_t mipLevel);

            void FinalizeAsyncUpload(uint16_t newStreamedMipLevels);

            void SetUploadHandle(const RHI::AsyncWorkHandle& handle);
            const RHI::AsyncWorkHandle& GetUploadHandle() const;

            using ImageOwnerProperty = RHI::ImageProperty<QueueId>;
            using SubresourceRangeOwner = ImageOwnerProperty::PropertyRange;

            AZStd::vector<SubresourceRangeOwner> GetOwnerQueue(const RHI::ImageSubresourceRange* range = nullptr) const;
            AZStd::vector<SubresourceRangeOwner> GetOwnerQueue(const RHI::ImageView& view) const;
            void SetOwnerQueue(const QueueId& queueId, const RHI::ImageSubresourceRange* range = nullptr);
            void SetOwnerQueue(const QueueId& queueId, const RHI::ImageView& view);

            using ImageLayoutProperty = RHI::ImageProperty<VkImageLayout>;
            using SubresourceRangeLayout = ImageLayoutProperty::PropertyRange;

            AZStd::vector<SubresourceRangeLayout> GetLayout(const RHI::ImageSubresourceRange* range = nullptr) const;
            void SetLayout(VkImageLayout layout, const RHI::ImageSubresourceRange* range = nullptr);

        private:
            Image() = default;

            RHI::ResultCode Init(Device& device, const RHI::ImageDescriptor& descriptor);
            RHI::ResultCode BindMemoryView(const MemoryView& memoryView, const RHI::HeapMemoryLevel heapMemoryLevel);

            RHI::ResultCode BuildNativeImage();
            VkImageFormatProperties GetImageFormatProperties() const;

            VkImageCreateFlags GetImageCreateFlags() const;
            VkImageUsageFlags GetImageUsageFlags() const;

            //////////////////////////////////////////////////////////////////////////
            // RHI::Image
            void SetDescriptor(const RHI::ImageDescriptor& descriptor) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::Resource
            void ReportMemoryUsage(RHI::MemoryStatisticsBuilder& builder) const override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::Image
            void GetSubresourceLayoutsInternal(
                const RHI::ImageSubresourceRange& subresourceRange,
                RHI::ImageSubresourceLayoutPlaced* subresourceLayouts,
                size_t* totalSizeInBytes) const override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////

            void GenerateSubresourceLayouts();

            void SetInitalQueueOwner();

            VkImage m_vkImage = VK_NULL_HANDLE;
            bool m_isOwnerOfNativeImage = true;
            MemoryView m_memoryView;
            VkMemoryRequirements m_memoryRequirements{};

            // Size in bytes in DeviceMemory.  Used from StreamingImagePool.
            size_t m_residentSizeInBytes = 0;

            // Tracking the actual mip level data uploaded. It's also used for invalidate image view. 
            uint16_t m_streamedMipLevel = 0;

            // Handle for uploading mip map data to the image.
            RHI::AsyncWorkHandle m_uploadHandle;

            // Queues that owns the image subresources.
            ImageOwnerProperty m_ownerQueue;
            mutable AZStd::mutex m_ownerQueueMutex;

            // Layout of image subresources.
            ImageLayoutProperty m_layout;
            mutable AZStd::mutex m_layoutMutex;
        };        
    }
}
