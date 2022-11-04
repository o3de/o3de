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
#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/ImageDescriptor.h>
#include <Atom/RHI.Reflect/ImageSubresource.h>
#include <Atom/RHI.Reflect/Size.h>
#include <Atom/RHI.Reflect/Vulkan/ImagePoolDescriptor.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/sort.h>
#include <RHI/Device.h>
#include <RHI/MemoryAllocator.h>
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
        class AliasedHeap;
        class Device;
        class Fence;
        class ImagePool;
        class StreamingImagePool;

        // contains all the information of the sparse image which includes memory bindings
        // To-do: add implementation to support non-color sparse image such as depth-stencil image
        struct SparseImageInfo
        {
            void Init(const Device& device, VkImage vkImage, const RHI::ImageDescriptor& imageDescriptor);

            // Memory requirements for color aspect only.
            VkSparseImageMemoryRequirements m_sparseImageMemoryRequirements;

            AZStd::vector<RHI::Size> m_mipSizes;
            
            VkImage m_image;

            // block size in bytes
            uint32_t m_blockSizeInBytes;
            // block count for the mip tail
            uint32_t m_mipTailBlockCount;
            // block count for each non-tail mip level
            AZStd::vector<uint32_t> m_mipBlockCount;

            // Memory bind for non-tail mips
            using MipMemoryViews = AZStd::vector<MemoryView>;
            AZStd::vector<MipMemoryViews> m_mipMemoryViews;                         // Memory views for each non-tail mip levels. For example: m_mipMemoryViews[0] is the memory views for mip level 0 (with all array layers)
            using MipMemoryBinds = AZStd::vector<VkSparseImageMemoryBind>;
            AZStd::vector<MipMemoryBinds> m_mipMemoryBinds;                         // Memory binds for each non-tail mip levels
            AZStd::vector<VkSparseImageMemoryBindInfo> m_mipMemoryBindInfos;        // Helper structure for sparse binding

            // Memory bind for mip tail
            uint16_t m_tailStartMip;                                                // First mip level in mip tail. mip levels from m_mipTailStart to (mipmap count - 1) are all belong to mip tail
            MemoryView m_mipTailMemoryView;
            VkSparseMemoryBind m_mipTailMemoryBind;                                // Opaque memory bindings for mip tail
            VkSparseImageOpaqueMemoryBindInfo m_mipTailMemoryBindInfo;              // Helper structure for sparse binding

            uint64_t GetRequiredMemorySize(uint16_t residentMipLevel) const;

            // Set the memory view used for mip tail
            void SetMipTailMemoryBind(const MemoryView& memoryView);

            // Add a memory view for a certain mip level
            void AddMipMemoryView(uint16_t mipLevel, const MemoryView& memoryView);
            // Remove all the memory views associated with a certain mip level
            void ResetMipMemoryView(uint16_t mipLevel);

            // Get the VkBindSparseInfo data for bind memory for specified mip range ( startMipLevel >= endMipLevel)
            VkBindSparseInfo GetBindSparseInfo(uint16_t startMipLevel, uint16_t endMipLevel);

            // Update the VkSparseImageMemoryBindInfo for a specified mip level
            void UpdateMipMemoryBindInfo(uint16_t mipLevel);

            // Update the VkSparseImageMemoryBindInfo for mip tail
            void UpdateMipTailMemoryBindInfo();
        };

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

            VkImage GetNativeImage() const;

            bool IsOwnerOfNativeImage() const;
            bool IsSparse() const;

            // get required memory size with minimum resident mip level
            VkMemoryRequirements GetMemoryRequirements(uint16_t residentMipLevel) const;

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

            RHI::ResultCode Init(Device& device, const RHI::ImageDescriptor& descriptor, bool tryUseSparse);
            RHI::ResultCode BindMemoryView(const MemoryView& memoryView, const RHI::HeapMemoryLevel heapMemoryLevel);

            // Allocate memory and bind memory for this image
            RHI::ResultCode AllocateAndBindMemory(MemoryAllocateFunction allocFunc, uint16_t residentMipLevel);

            RHI::ResultCode BuildNativeImage();

            RHI::ResultCode BuildSparseImage();

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

            // for sparse residency image if it's supported
            bool m_isSparse = false;
            AZStd::unique_ptr<SparseImageInfo> m_sparseImageInfo;

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
