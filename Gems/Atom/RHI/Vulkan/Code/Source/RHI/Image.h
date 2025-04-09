/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceImage.h>
#include <Atom/RHI/ImageProperty.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/ImageDescriptor.h>
#include <Atom/RHI.Reflect/ImageSubresource.h>
#include <Atom/RHI.Reflect/Size.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/sort.h>
#include <RHI/MemoryView.h>
#include <RHI/Queue.h>
#include <RHI/VulkanMemoryAllocation.h>
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
            const static uint32_t StandardBlockSize = 64*1024;

            void Init(const Device& device, VkImage vkImage, const RHI::ImageDescriptor& imageDescriptor);

            // Memory requirements for color aspect only.
            VkSparseImageMemoryRequirements m_sparseImageMemoryRequirements;

            VkImage m_image;

            // block size in bytes
            size_t m_blockSizeInBytes;

            // Whether the image uses single mip tail for all array layers (VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT was set)
            bool m_useSingleMipTail = false;

            // Block count for one mip tail.
            // If the image is single mip tail, then it's the block count for the single mip tail
            // It the image is not single mip tail, then it's the block count for the mip tail of one array layer
            uint32_t m_mipTailBlockCount;

            // Memory binding data for one non-tail mip
            using MultiHeapTiles = AZStd::vector<RHI::Ptr<VulkanMemoryAllocation>>; 
            using MemoryViews = AZStd::vector<MemoryView>;
            using MipMemoryBinds = AZStd::vector<VkSparseImageMemoryBind>;
            struct NonTailMipInfo
            {
                // basic info
                RHI::Size m_size;
                uint32_t m_blockCount;

                // Allocated memory blocks and corresponding bindings
                MultiHeapTiles m_heapTiles;                             // Allocated memory tiles (from multiple heaps) for this non-tail mip level.
                MipMemoryBinds m_memoryBinds;                           // All the sparse memory binds for this mip level

                // Cached structures for unbound memory. Used for unbound certain non-tail mip levels
                MipMemoryBinds m_emptyMemoryBinds;                      // Empty binds
            };

            // Memory binding info for each non-tail mip levels
            AZStd::vector<NonTailMipInfo> m_nonTailMipInfos;

            // Helper data for generating the final VkBindSparseInfo
            // Note: We need the these helper info structure data to be continues for setup VkBindSparseInfo for more than one mipmaps
            AZStd::vector<VkSparseImageMemoryBindInfo> m_mipMemoryBindInfos;
            AZStd::vector<VkSparseImageMemoryBindInfo> m_emptyMipMemoryBindInfos;    // Bind info pointer to the empty binds
           
            // Memory bind for mip tail
            // (Note: mip tail uses different Vulkan memory bind structure than non-tail mips)
            uint16_t m_tailStartMip;                                                // First mip level in mip tail. mip levels from m_mipTailStart to (mipmap count - 1) are all belong to mip tail
            MultiHeapTiles m_mipTailHeapTiles;
            AZStd::vector<VkSparseMemoryBind> m_mipTailMemoryBinds;                 // Opaque memory bindings for mip tail
            VkSparseImageOpaqueMemoryBindInfo m_mipTailMemoryBindInfo;              // Helper structure for sparse binding
            
            // helper function to convert MultiHeapTiles to MemoryViews
            static void MultiHeapTilesToMemoryViews(MemoryViews& output, const MultiHeapTiles& multiHeapTiles);

            uint64_t GetRequiredMemorySize(uint16_t residentMipLevel) const;

            // Get the VkBindSparseInfo data for bind memory for specified mip range ( startMipLevel >= endMipLevel)
            VkBindSparseInfo GetBindSparseInfo(uint16_t startMipLevel, uint16_t endMipLevel);

            // Update the VkSparseImageMemoryBindInfo for a specified mip level
            void UpdateMipMemoryBindInfo(uint16_t mipLevel);

            // Update the VkSparseImageMemoryBindInfo for mip tail
            void UpdateMipTailMemoryBindInfo();

            // Update the sparse image memory bind info for specified mip range ( startMipLevel >= endMipLevel)
            void UpdateMemoryBindInfo(uint16_t startMipLevel, uint16_t endMipLevel);
        };

        class Image final
            : public RHI::DeviceImage
        {
            using Base = RHI::DeviceImage;
            friend class ImagePool;
            friend class StreamingImagePool;
            friend class AliasedHeap;
            friend class Device;
            friend class SwapChain;

        public:
            AZ_CLASS_ALLOCATOR(Image, AZ::SystemAllocator);
            AZ_RTTI(Image, "725F56BF-5CCA-4110-91EE-C94E84A35B2C", Base);

            static RHI::Ptr<Image> Create();

            ~Image();
            
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
            AZStd::vector<SubresourceRangeOwner> GetOwnerQueue(const RHI::DeviceImageView& view) const;
            void SetOwnerQueue(const QueueId& queueId, const RHI::ImageSubresourceRange* range = nullptr);
            void SetOwnerQueue(const QueueId& queueId, const RHI::DeviceImageView& view);

            using ImageLayoutProperty = RHI::ImageProperty<VkImageLayout>;
            using SubresourceRangeLayout = ImageLayoutProperty::PropertyRange;

            AZStd::vector<SubresourceRangeLayout> GetLayout(const RHI::ImageSubresourceRange* range = nullptr) const;
            void SetLayout(VkImageLayout layout, const RHI::ImageSubresourceRange* range = nullptr);

            using ImagePipelineAccessProperty = RHI::ImageProperty<PipelineAccessFlags>;

            PipelineAccessFlags GetPipelineAccess(const RHI::ImageSubresourceRange* range = nullptr) const;
            void SetPipelineAccess(const PipelineAccessFlags& pipelineAccess, const RHI::ImageSubresourceRange* range = nullptr);

            VkImageUsageFlags GetUsageFlags() const;

            VkSharingMode GetSharingMode() const;

            //! Flags used for initializing an image
            enum class InitFlags : uint32_t
            {
                None = 0,
                TrySparse = AZ_BIT(0)  // Try to create a sparse image first
            };

            const MemoryView& GetMemoryView();

        private:
            Image() = default;

            RHI::ResultCode Init(Device& device, VkImage image, const RHI::ImageDescriptor& descriptor); // It is internally used to handle Swapchain images as Vulkan::Image class.
            RHI::ResultCode Init(Device& device, const RHI::ImageDescriptor& descriptor, const InitFlags flags);
            RHI::ResultCode Init(Device& device, const RHI::ImageDescriptor& descriptor, const MemoryView& memoryView);

            //! Common initialization for all Init functions (init variables, set desciptor, set initflags, init DeviceObject, etc)
            void OnPreInit(Device& device, const RHI::ImageDescriptor& descriptor, InitFlags flags);
            //! Common post initialization for all Init functions (set name, get memory requirements, etc)
            void OnPostInit();

            // Allocate memory and bind memory which can store mips up to residentMipLevel 
            RHI::ResultCode AllocateAndBindMemory(StreamingImagePool& imagePool, uint16_t residentMipLevel);

            // Create a VkImage which only requires regular (one time) memory binding
            RHI::ResultCode BuildNativeImage(const MemoryView* memoryView = nullptr);

            // Create a VkImage with sparse memory binding support
            RHI::ResultCode BuildSparseImage();

            void ReleaseAllMemory(StreamingImagePool& imagePool);

            // Trim image to specified mip level. Release unused bound memory if updateMemoryBind is true
            RHI::ResultCode TrimImage(StreamingImagePool& imagePool, uint16_t targetMipLevel, bool updateMemoryBind);

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceImage
            void SetDescriptor(const RHI::ImageDescriptor& descriptor) override;
            bool IsStreamableInternal() const override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceResource
            void ReportMemoryUsage(RHI::MemoryStatisticsBuilder& builder) const override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceImage
            void GetSubresourceLayoutsInternal(
                const RHI::ImageSubresourceRange& subresourceRange,
                RHI::DeviceImageSubresourceLayout* subresourceLayouts,
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

            // The highest mip level that the image's current bound memory can accommodate
            // Todo: need a better name
            uint16_t m_highestMipLevel = RHI::Limits::Image::MipCountMax;

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

            // Last pipeline access to the image subresources
            ImagePipelineAccessProperty m_pipelineAccess;
            mutable AZStd::mutex m_pipelineAccessMutex;

            // Usage flags used for creating the image.
            VkImageUsageFlags m_usageFlags;

            VkSharingMode m_sharingMode;

            // Flags used for initializing the image.
            InitFlags m_initFlags = InitFlags::None;
        };

        AZ_DEFINE_ENUM_BITWISE_OPERATORS(Image::InitFlags);
    }
}
