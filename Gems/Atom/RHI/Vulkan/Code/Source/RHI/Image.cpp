/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/std/algorithm.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/sort.h>
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <RHI/AsyncUploadQueue.h>
#include <RHI/Device.h>
#include <RHI/Fence.h>
#include <RHI/Image.h>
#include <RHI/ImagePool.h>
#include <RHI/ImageView.h>
#include <RHI/PhysicalDevice.h>
#include <RHI/Queue.h>
#include <RHI/ReleaseContainer.h>
#include <RHI/StreamingImagePool.h>
#include <RHI/SwapChain.h>
#include <Atom/RHI.Reflect/VkAllocator.h>

namespace AZ
{
    namespace Vulkan
    {
        // SparseImageInfo functions
        void SparseImageInfo::Init(const Device& device, VkImage vkImage, const RHI::ImageDescriptor& imageDescriptor)
        {
            // Get image memory requirements 
            VkMemoryRequirements memoryRequirements;
            const GladVulkanContext& vulkanContext = device.GetContext();
            VkDevice vkDevice = device.GetNativeDevice();
            vulkanContext.GetImageMemoryRequirements(vkDevice, vkImage, &memoryRequirements);
            
            // get sparse memory requirement count;
            uint32_t sparseMemoryReqsCount = 0;
            vulkanContext.GetImageSparseMemoryRequirements(vkDevice, vkImage, &sparseMemoryReqsCount, nullptr);
            AZ_Assert(sparseMemoryReqsCount, "Sparse memory requirements count shouldn't be 0");

            // Get actual requirements
            AZStd::vector<VkSparseImageMemoryRequirements> sparseImageMemoryRequirements;
            sparseImageMemoryRequirements.resize(sparseMemoryReqsCount);
            vulkanContext.GetImageSparseMemoryRequirements(vkDevice, vkImage, &sparseMemoryReqsCount, sparseImageMemoryRequirements.data());

            bool validSparseImage = false;
            for (const VkSparseImageMemoryRequirements& requirements : sparseImageMemoryRequirements)
            {
                if (requirements.formatProperties.aspectMask == VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT)
                {
                    m_sparseImageMemoryRequirements = requirements;
                    validSparseImage = true;
                    break;
                }
            }

            if (!validSparseImage)
            {
                AZ_Assert(false, "Non-color sparse image is not supported");
                return;
            }

            m_image = vkImage;

            VkExtent3D imageGranularity = m_sparseImageMemoryRequirements.formatProperties.imageGranularity;

            m_blockSizeInBytes = memoryRequirements.alignment;

            m_useSingleMipTail = m_sparseImageMemoryRequirements.formatProperties.flags & VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT;

            // calculate block count for one mip tail
            m_mipTailBlockCount = aznumeric_cast<uint32_t>(m_sparseImageMemoryRequirements.imageMipTailSize/m_blockSizeInBytes);
            
            // calculate block count for each non-tail mip levels
            m_tailStartMip =  aznumeric_cast<uint16_t>(m_sparseImageMemoryRequirements.imageMipTailFirstLod);
            uint32_t nonTailMipLevels = m_tailStartMip;
            if (nonTailMipLevels)
            {
                m_mipMemoryBindInfos.resize(nonTailMipLevels);
                m_emptyMipMemoryBindInfos.resize(nonTailMipLevels);
                m_nonTailMipInfos.resize(nonTailMipLevels);

                for (uint32_t mip = 0; mip < nonTailMipLevels; mip++)
                {
                    NonTailMipInfo& mipInfo = m_nonTailMipInfos[mip];
                    mipInfo.m_size = imageDescriptor.m_size.GetReducedMip(mip);
                    mipInfo.m_blockCount = AZ::DivideAndRoundUp(mipInfo.m_size.m_width, imageGranularity.width)
                        * AZ::DivideAndRoundUp(mipInfo.m_size.m_height, imageGranularity.height)
                        * AZ::DivideAndRoundUp(mipInfo.m_size.m_depth, imageGranularity.depth);

                    // fill empty bounds data
                    mipInfo.m_emptyMemoryBinds.resize(imageDescriptor.m_arraySize);
                    for (uint16_t arrayIndex = 0; arrayIndex < imageDescriptor.m_arraySize; arrayIndex++)
                    {
                        VkSparseImageMemoryBind& emptyBind = mipInfo.m_emptyMemoryBinds[arrayIndex];
                        emptyBind.subresource.mipLevel = mip;
                        emptyBind.subresource.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
                        emptyBind.subresource.arrayLayer = arrayIndex;
                        emptyBind.memory = VK_NULL_HANDLE;
                        emptyBind.offset.x = 0;
                        emptyBind.offset.y = 0;
                        emptyBind.offset.z = 0;
                        emptyBind.extent.width = mipInfo.m_size.m_width;
                        emptyBind.extent.height = mipInfo.m_size.m_height;
                        emptyBind.extent.depth = mipInfo.m_size.m_depth;
                        emptyBind.flags = 0;
                        emptyBind.memoryOffset = 0;
                    }

                    VkSparseImageMemoryBindInfo& emptyBindInfo = m_emptyMipMemoryBindInfos[mip];
                    emptyBindInfo.bindCount = aznumeric_caster(mipInfo.m_emptyMemoryBinds.size());
                    emptyBindInfo.pBinds = mipInfo.m_emptyMemoryBinds.data();
                    emptyBindInfo.image = m_image;
                }
            }
        }

        void SparseImageInfo::MultiHeapTilesToMemoryViews(MemoryViews& output, const MultiHeapTiles& multiHeapTiles)
        {
            for (const HeapTiles& heapTiles : multiHeapTiles)
            {
                for (const RHI::PageTileSpan& tileSpan : heapTiles.m_tileSpanList)
                {
                    MemoryView memoryView { heapTiles.m_heap, tileSpan.m_offset * SparseImageInfo::StandardBlockSize,
                        tileSpan.m_tileCount * SparseImageInfo::StandardBlockSize, SparseImageInfo::StandardBlockSize, MemoryAllocationType::SubAllocated };
                    output.push_back(memoryView);
                }
            }
        }

        uint64_t SparseImageInfo::GetRequiredMemorySize(uint16_t residentMipLevel) const
        {
            // minimum memory size should be mip tail size
            uint64_t memorySize = m_sparseImageMemoryRequirements.imageMipTailSize;

            // for any resident mips which are not part of tail
            for (uint32_t mip = residentMipLevel; mip < m_tailStartMip; mip++ )
            {
                memorySize += m_nonTailMipInfos[mip].m_blockCount * m_blockSizeInBytes;
            }

            return memorySize; 
        }

        void SparseImageInfo::UpdateMipMemoryBindInfo(uint16_t mipLevel)
        {
            AZ_Assert(mipLevel < m_tailStartMip, "Invalid mip level. Mip level should be smaller than m_tailStartMip");
            
            NonTailMipInfo& mipInfo = m_nonTailMipInfos[mipLevel];

            MemoryViews memoryViews;
            MultiHeapTilesToMemoryViews(memoryViews, mipInfo.m_heapTiles);

            size_t memoryViewCount = memoryViews.size();
            auto imageGranularity = m_sparseImageMemoryRequirements.formatProperties.imageGranularity;
            MipMemoryBinds& memoryBinds = mipInfo.m_memoryBinds;
            memoryBinds.clear();

            if (memoryViewCount == 0)
            {
                VkSparseImageMemoryBindInfo& info = m_mipMemoryBindInfos[mipLevel];
                info.image = nullptr;
                info.bindCount = 0;
                info.pBinds = nullptr;
                return;
            }

            // subresource for this mip level
            VkImageSubresource subResource;
            subResource.mipLevel = mipLevel;
            subResource.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
            subResource.arrayLayer = 0;

            RHI::Size mipSize = mipInfo.m_size;
            const uint32_t mipBlockCount = mipInfo.m_blockCount;
            const uint32_t blockCountPerRow = AZ::DivideAndRoundUp(mipSize.m_width, imageGranularity.width);
            const uint32_t blockCountPerColumn = AZ::DivideAndRoundUp(mipSize.m_height, imageGranularity.height);

            // convert memory views to memory binds
            uint32_t boundBlockCount = 0;
            for (size_t index = 0; index < memoryViewCount; index++)
            {
                const MemoryView& memoryView = memoryViews[index];
                VkSparseImageMemoryBind memoryBind;
                
                uint32_t memoryBlockCount = aznumeric_cast<uint32_t>(memoryView.GetSize() / m_blockSizeInBytes);

                // No more than one row of blocks per bind for simpler implementation
                while (memoryBlockCount > 0)
                {
                    // find the offset base on bound blocks
                    VkOffset3D offset;
                    offset.x = (boundBlockCount % blockCountPerRow) * imageGranularity.width;
                    offset.y = (boundBlockCount / blockCountPerRow) * imageGranularity.height;
                    offset.z = (boundBlockCount / (blockCountPerRow * blockCountPerColumn)) * imageGranularity.depth;

                    // only update the width of the extent 
                    uint32_t bindCount = AZStd::min(blockCountPerRow - boundBlockCount%blockCountPerRow, memoryBlockCount);
                    VkExtent3D extent {bindCount * imageGranularity.width, imageGranularity.height, imageGranularity.depth};
                    // handle the edges
                    if (offset.x + extent.width > mipSize.m_width)
                    {
                        extent.width = mipSize.m_width - offset.x;
                    }
                    if (offset.y + extent.height > mipSize.m_height)
                    {
                        extent.height = mipSize.m_height - offset.y;
                    }
                    if (offset.z + extent.depth > mipSize.m_depth)
                    {
                        extent.depth = mipSize.m_depth - offset.z;
                    }
               
                    memoryBind.subresource = subResource;
                    memoryBind.memory = memoryView.GetMemory()->GetNativeDeviceMemory();
                    memoryBind.memoryOffset = memoryView.GetOffset() + memoryView.GetSize() - memoryBlockCount * m_blockSizeInBytes;
                    memoryBind.offset = offset;
                    memoryBind.extent = extent;
                    memoryBind.flags = 0;
                    memoryBinds.push_back(memoryBind);

                    memoryBlockCount -= bindCount;
                    boundBlockCount += bindCount;

                    // move to next array layer
                    if (boundBlockCount == mipBlockCount)
                    {
                        subResource.arrayLayer++;
                        boundBlockCount = 0;
                    }

                    if (boundBlockCount > mipBlockCount)
                    {
                        AZ_Assert(false, "unexpected bound count");
                    }
                }
            }

            // generate bind info
            VkSparseImageMemoryBindInfo& info = m_mipMemoryBindInfos[mipLevel];
            info.image = m_image;
            info.bindCount = aznumeric_cast<uint32_t>(memoryBinds.size());
            info.pBinds = memoryBinds.data();
        }

        void SparseImageInfo::UpdateMipTailMemoryBindInfo()
        {
            m_mipTailMemoryBinds.clear();

            // convert heap tiles to memory views
            MemoryViews memoryViews;
            MultiHeapTilesToMemoryViews(memoryViews, m_mipTailHeapTiles);

            if (memoryViews.size() == 0)
            {
                m_mipTailMemoryBindInfo.bindCount = 0;
                m_mipTailMemoryBindInfo.image = VK_NULL_HANDLE;
                m_mipTailMemoryBindInfo.pBinds = nullptr;

                return;
            }

            uint32_t arrayIndex = 0;
            // bound block count for current array layer
            uint32_t boundBlockCount = 0;

            // it's possible there are multiple memory views bound to one array layer mip tail
            // It's also possible one memory view have blocks bound to multiple array layers of mip tail.
            // Each array layer need to be bound separately.
            for (const MemoryView& memoryView : memoryViews)
            {
                // blocks in the MemoryView which are available to bind
                uint32_t memoryBlockCount = aznumeric_cast<uint32_t>(memoryView.GetSize() / m_blockSizeInBytes);

                while (memoryBlockCount > 0)
                {
                    // block count to be bound in this VkSparseMemoryBind
                    uint32_t bindCount = AZStd::min(memoryBlockCount, m_mipTailBlockCount - boundBlockCount);

                    VkSparseMemoryBind memoryBind;
                    memoryBind.memory = memoryView.GetMemory()->GetNativeDeviceMemory();
                    memoryBind.memoryOffset = memoryView.GetOffset() + memoryView.GetSize() - memoryBlockCount*m_blockSizeInBytes;
                    memoryBind.size = bindCount * m_blockSizeInBytes;
                    memoryBind.resourceOffset = boundBlockCount * m_blockSizeInBytes + m_sparseImageMemoryRequirements.imageMipTailOffset
                        + arrayIndex * m_sparseImageMemoryRequirements.imageMipTailStride;
                    memoryBind.flags = 0;

                    m_mipTailMemoryBinds.push_back(memoryBind);

                    memoryBlockCount -= bindCount;
                    boundBlockCount += bindCount;

                    // move to next array layer
                    if (boundBlockCount == m_mipTailBlockCount)
                    {
                        arrayIndex++;
                        boundBlockCount = 0;
                    }

                    if (boundBlockCount > m_mipTailBlockCount)
                    {
                        AZ_Assert(false, "unexpected bound count");
                    }
                }
            }

            m_mipTailMemoryBindInfo.bindCount = aznumeric_cast<uint32_t>(m_mipTailMemoryBinds.size());
            m_mipTailMemoryBindInfo.image = m_image;
            m_mipTailMemoryBindInfo.pBinds = m_mipTailMemoryBinds.data();
        }

        void SparseImageInfo::UpdateMemoryBindInfo(uint16_t startMipLevel, uint16_t endMipLevel)
        {
            AZ_Assert(startMipLevel >= endMipLevel, "Invalid mip level range; start mip level shouldn't be smaller than endMipLevel");
            if (startMipLevel >= m_tailStartMip)
            {
                UpdateMipTailMemoryBindInfo();
            }

            if (m_tailStartMip > 0 && endMipLevel < m_tailStartMip)
            {
                uint16_t startMip = AZStd::min(startMipLevel, aznumeric_cast<uint16_t>(m_tailStartMip - 1));
                for (uint16_t mip = endMipLevel; mip <= startMip; mip++)
                {
                    UpdateMipMemoryBindInfo(mip);
                }
            }
        }

        VkBindSparseInfo SparseImageInfo::GetBindSparseInfo(uint16_t startMipLevel, uint16_t endMipLevel)
        {
            AZ_Assert(startMipLevel >= endMipLevel, "Invalid mip level range; start mip level shouldn't be smaller than endMipLevel");

            VkBindSparseInfo bindSparseInfo;
            bindSparseInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;
            bindSparseInfo.pNext = nullptr;
            bindSparseInfo.waitSemaphoreCount = 0;
            bindSparseInfo.pWaitSemaphores = nullptr;
            bindSparseInfo.signalSemaphoreCount = 0;
            bindSparseInfo.pSignalSemaphores = nullptr;
            bindSparseInfo.bufferBindCount = 0;
            bindSparseInfo.pBufferBinds = nullptr;

            // add mip tail binds if start mip level is part of mip tail
            if (startMipLevel >= m_tailStartMip && m_mipTailBlockCount > 0)
            {
                bindSparseInfo.imageOpaqueBindCount = 1;
                bindSparseInfo.pImageOpaqueBinds = &m_mipTailMemoryBindInfo;
            }
            else
            {
                bindSparseInfo.imageOpaqueBindCount = 0;
                bindSparseInfo.pImageOpaqueBinds = nullptr;
            }

            // non-tail mips
            bindSparseInfo.imageBindCount = 0;
            bindSparseInfo.pImageBinds = nullptr;
            if (m_tailStartMip > 0 && endMipLevel < m_tailStartMip)
            {
                uint16_t startMip = AZStd::min(startMipLevel, aznumeric_cast<uint16_t>(m_tailStartMip - 1));
                if (startMip >= endMipLevel)
                {
                    bindSparseInfo.imageBindCount = startMip - endMipLevel + 1;
                    if (bindSparseInfo.imageBindCount > 0)
                    {
                        NonTailMipInfo& mipInfo = m_nonTailMipInfos[startMip];
                        if (mipInfo.m_memoryBinds.empty())
                        {
                            bindSparseInfo.pImageBinds = m_emptyMipMemoryBindInfos.data() + endMipLevel;
                        }
                        else
                        {
                            bindSparseInfo.pImageBinds = m_mipMemoryBindInfos.data() + endMipLevel;
                        }
                    }
                }
            }

            return bindSparseInfo;
        }

        Image::~Image()
        {
            Invalidate();
        }

        RHI::Ptr<Image> Image::Create()
        {
            return aznew Image();
        }

         RHI::ResultCode Image::Init(Device& device, VkImage image, const RHI::ImageDescriptor& descriptor)
        {
            AZ_Assert(image != VK_NULL_HANDLE, "Vulkan Image is null.");
            RHI::DeviceObject::Init(device);
            m_vkImage = image;
            m_isOwnerOfNativeImage = false;
            SetDescriptor(descriptor);
            SetName(GetName());
            return RHI::ResultCode::Success;
        }

        void Image::Invalidate()
        {
            if (m_vkImage != VK_NULL_HANDLE && m_isOwnerOfNativeImage)
            {
                auto& device = static_cast<Device&>(GetDevice());
                device.QueueForRelease(
                    new ReleaseContainer<VkImage>(device.GetNativeDevice(), m_vkImage, device.GetContext().DestroyImage));
                // ensure memory is released
                AZ_Assert(!m_memoryView.IsValid(), "Memory should be released before Invalidate() is called");
            }
            m_vkImage = VK_NULL_HANDLE;

            m_highestMipLevel= RHI::Limits::Image::MipCountMax;
            m_residentSizeInBytes = 0;

            m_layout.Reset();
            m_ownerQueue.Reset();
            m_isSparse = false;
            m_sparseImageInfo.reset();
        }

        VkImage Image::GetNativeImage() const
        {
            return m_vkImage;
        }

        bool Image::IsOwnerOfNativeImage() const
        {
            return m_isOwnerOfNativeImage;
        }

        bool Image::IsSparse() const
        {
            return m_isSparse;
        }

        VkMemoryRequirements Image::GetMemoryRequirements(uint16_t residentMipLevel) const
        {
            if (m_isSparse)
            {
                VkMemoryRequirements requirements = m_memoryRequirements;
                requirements.size = m_sparseImageInfo->GetRequiredMemorySize(residentMipLevel);
                return requirements;
            }
            else
            {
                return m_memoryRequirements;
            }
        }

        VkImageAspectFlags Image::GetImageAspectFlags() const
        {
            return ConvertImageAspectFlags(GetAspectFlags());
        }

        size_t Image::GetResidentSizeInBytes() const
        {
            return m_residentSizeInBytes;
        }

        void Image::SetResidentSizeInBytes(size_t sizeInBytes)
        {
            m_residentSizeInBytes = sizeInBytes;
        }

        uint16_t Image::GetStreamedMipLevel() const
        {
            return m_streamedMipLevel;
        }

        void Image::SetStreamedMipLevel(uint16_t mipLevel)
        {
            m_streamedMipLevel = mipLevel;
        }

        void Image::FinalizeAsyncUpload(uint16_t newStreamedMipLevels)
        {
            AZ_Assert(newStreamedMipLevels <= m_streamedMipLevel, "Expand mip levels can't be more than streamed mip level.");

            if (newStreamedMipLevels < m_streamedMipLevel)
            {
                m_streamedMipLevel = newStreamedMipLevels;
                InvalidateViews();
            }
        }

        void Image::SetUploadHandle(const RHI::AsyncWorkHandle& handle)
        {
            m_uploadHandle = handle;
        }

        const RHI::AsyncWorkHandle& Image::GetUploadHandle() const
        {
            return m_uploadHandle;
        }

        AZStd::vector<Image::SubresourceRangeOwner> Image::GetOwnerQueue(const RHI::ImageSubresourceRange* range /*= nullptr*/) const
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_ownerQueueMutex);
            return m_ownerQueue.Get(range ? *range : RHI::ImageSubresourceRange(GetDescriptor()));
        }

        AZStd::vector<Image::SubresourceRangeOwner> Image::GetOwnerQueue(const RHI::ImageView& view) const
        {
            auto range = RHI::ImageSubresourceRange(view.GetDescriptor());
            return GetOwnerQueue(&range);
        }

        void Image::SetOwnerQueue(const QueueId& queueId, const RHI::ImageSubresourceRange* range /*= nullptr*/)
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_ownerQueueMutex);
            m_ownerQueue.Set(range ? *range : RHI::ImageSubresourceRange(GetDescriptor()), queueId);
        }

        void Image::SetOwnerQueue(const QueueId& queueId, const RHI::ImageView& view)
        {
            auto range = RHI::ImageSubresourceRange(view.GetDescriptor());
            SetOwnerQueue(queueId , &range);
        }

        AZStd::vector<Image::SubresourceRangeLayout> Image::GetLayout(const RHI::ImageSubresourceRange* range /*= nullptr*/) const
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_layoutMutex);
            return m_layout.Get(range ? *range : RHI::ImageSubresourceRange(GetDescriptor()));
        }

        void Image::SetLayout(VkImageLayout layout, const RHI::ImageSubresourceRange* range /*= nullptr*/)
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_layoutMutex);
            m_layout.Set(range ? *range : RHI::ImageSubresourceRange(GetDescriptor()), layout);
        }

        VkImageUsageFlags Image::GetUsageFlags() const
        {
            return m_usageFlags;
        }

        RHI::ResultCode Image::Init(Device& device, const RHI::ImageDescriptor& descriptor, bool tryUseSparse)
        {
            RHI::DeviceObject::Init(device);
            SetDescriptor(descriptor);

            m_isSparse = false;

            // try create sparse image first
            if (tryUseSparse)
            {
                m_isSparse = BuildSparseImage() == RHI::ResultCode::Success;
            }

            if (!m_isSparse)
            {
                // create non-sparse if failed to create sparse image
                RHI::ResultCode result = BuildNativeImage();
                RETURN_RESULT_IF_UNSUCCESSFUL(result);
            }

            m_isOwnerOfNativeImage = true;

            // Get image memory requirements 
            device.GetContext().GetImageMemoryRequirements(device.GetNativeDevice(), m_vkImage, &m_memoryRequirements);

            SetName(GetName());

            return RHI::ResultCode::Success;
        }

        RHI::ResultCode Image::TrimImage(StreamingImagePool& pool, uint16_t targetMipLevel, bool updateMemoryBind)
        {
            // Set streamed mip level to target mip level if there are more mips
            if (m_streamedMipLevel < targetMipLevel)
            {
                m_streamedMipLevel = targetMipLevel;
                InvalidateViews();
            }

            AZ_Assert(m_highestMipLevel <= targetMipLevel, "Bound mip level doesn't contain target mip level");

            // update memory binding 
            if (m_highestMipLevel < targetMipLevel && updateMemoryBind)
            {
                if (m_isSparse)
                {
                    // we can only unbound memory for non-tail mips
                    uint16_t adjustedTargetMipLevel = AZStd::min(m_sparseImageInfo->m_tailStartMip, targetMipLevel);
                    if (m_highestMipLevel < adjustedTargetMipLevel)
                    {
                        for (uint16_t mip = m_highestMipLevel; mip < adjustedTargetMipLevel; mip++)
                        {
                            // release memories
                            pool.DeAllocateMemoryBlocks(m_sparseImageInfo->m_nonTailMipInfos[mip].m_heapTiles);
                            m_sparseImageInfo->UpdateMipMemoryBindInfo(mip);
                        }

                        // unbound sparse memories for these mips
                        VkBindSparseInfo bindSparseInfo = m_sparseImageInfo->GetBindSparseInfo(adjustedTargetMipLevel-1, m_highestMipLevel);
                        m_highestMipLevel = adjustedTargetMipLevel;
                        m_residentSizeInBytes = m_sparseImageInfo->GetRequiredMemorySize(m_highestMipLevel);
                        auto& device = static_cast<Device&>(GetDevice());
                        device.GetAsyncUploadQueue().QueueBindSparse(bindSparseInfo);
                    }
                }
            }

            return RHI::ResultCode::Success;
        }

        RHI::ResultCode Image::BindMemoryView(const MemoryView& memoryView)
        {
            AZ_Assert(m_vkImage != VK_NULL_HANDLE, "Vulkan's native image is not initialized.");
            AZ_Assert(memoryView.IsValid(), "MemoryView is not valid.");
            AZ_Assert(!m_isSparse, "use bind memory view for sparse image.");

            auto& device = static_cast<Device&>(GetDevice());
            VkResult vkResult = device.GetContext().BindImageMemory(
                device.GetNativeDevice(), m_vkImage, memoryView.GetMemory()->GetNativeDeviceMemory(), memoryView.GetOffset());
            AssertSuccess(vkResult);
            RHI::ResultCode result = ConvertResult(vkResult);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);
            m_memoryView = memoryView;
            return result;
        }

        RHI::ResultCode Image::AllocateAndBindMemory(StreamingImagePool& imagePool, uint16_t residentMipLevel)
        {
            auto& device = static_cast<Device&>(GetDevice());
            RHI::ResultCode result = RHI::ResultCode::Success;

            // the bound memory is large enough for expected resident mip level
            if (residentMipLevel >= m_highestMipLevel)
            {
                return RHI::ResultCode::Success;
            }

            if (m_isSparse)
            {
                // The mip we need to start to allocate memory from
                const uint16_t startMip = m_highestMipLevel - 1;
                const uint16_t endMip = AZStd::min(residentMipLevel, m_sparseImageInfo->m_tailStartMip);

                // allocated memory views
                AZStd::vector<SparseImageInfo::MultiHeapTiles*> allocatedHeapTiles;

                // mip tail
                // Note: with certain use cases (usually image only has one mip) the m_sparseImageInfo->m_mipTailBlockCount could be zero
                if (startMip >= m_sparseImageInfo->m_tailStartMip && m_sparseImageInfo->m_mipTailBlockCount)
                {
                    uint32_t blockCount = m_sparseImageInfo->m_mipTailBlockCount;
                    if (!m_sparseImageInfo->m_useSingleMipTail)
                    {
                        blockCount *= GetDescriptor().m_arraySize;
                    }
                    
                    result = imagePool.AllocateMemoryBlocks(m_sparseImageInfo->m_mipTailHeapTiles, blockCount);
                    if (result != RHI::ResultCode::Success)
                    {
                        return result;
                    }

                    allocatedHeapTiles.push_back(&m_sparseImageInfo->m_mipTailHeapTiles);
                }

                // non-tail mips
                uint16_t nonTailStart = startMip;
                bool allocationSuccess = true;
                if (m_sparseImageInfo->m_tailStartMip > 0 && endMip < m_sparseImageInfo->m_tailStartMip)
                {
                    nonTailStart = AZStd::min(startMip, aznumeric_cast<uint16_t>(m_sparseImageInfo->m_tailStartMip-1));
                    for (uint16_t mipLevel = endMip; mipLevel <= nonTailStart; mipLevel++ )
                    {
                        SparseImageInfo::NonTailMipInfo& mipInfo = m_sparseImageInfo->m_nonTailMipInfos[mipLevel];
                        uint32_t blockCount = mipInfo.m_blockCount * GetDescriptor().m_arraySize;
                        result = imagePool.AllocateMemoryBlocks(mipInfo.m_heapTiles, blockCount);
                        if (result != RHI::ResultCode::Success)
                        {
                            allocationSuccess = false;
                            break;
                        }
                        allocatedHeapTiles.push_back(&mipInfo.m_heapTiles);
                    }
                }

                // release allocated memory views and return if any allocation was failed
                if (!allocationSuccess)
                {
                    for (SparseImageInfo::MultiHeapTiles* multiHeapTiles:allocatedHeapTiles)
                    {
                        imagePool.DeAllocateMemoryBlocks(*multiHeapTiles);
                    }
                    return result;
                }

                // update the memory bind info for the mips which were just allocated
                m_sparseImageInfo->UpdateMemoryBindInfo(startMip, endMip);

                // Queue the image's sparse binding
                VkBindSparseInfo bindSparseInfo = m_sparseImageInfo->GetBindSparseInfo(startMip, endMip);
                m_highestMipLevel = endMip;
                m_residentSizeInBytes = m_sparseImageInfo->GetRequiredMemorySize(m_highestMipLevel);
                device.GetAsyncUploadQueue().QueueBindSparse(bindSparseInfo);
            }
            else 
            {
                auto memoryView = imagePool.AllocateMemory(m_memoryRequirements.size, m_memoryRequirements.alignment);
                if (!memoryView.IsValid())
                {
                    return RHI::ResultCode::OutOfMemory;
                }

                VkResult vkResult = device.GetContext().BindImageMemory(
                    device.GetNativeDevice(), m_vkImage, memoryView.GetMemory()->GetNativeDeviceMemory(), memoryView.GetOffset());
                AssertSuccess(vkResult);
                result = ConvertResult(vkResult);
                RETURN_RESULT_IF_UNSUCCESSFUL(result);

                if (m_memoryView.IsValid())
                {
                    imagePool.DeAllocateMemory(m_memoryView);
                }

                m_memoryView = memoryView;
                m_residentSizeInBytes = m_memoryRequirements.size;
                m_highestMipLevel = 0;
            }

            return RHI::ResultCode::Success;
        }

        RHI::ResultCode Image::BuildSparseImage()
        {
            AZ_Assert(m_vkImage == VK_NULL_HANDLE, "Vulkan's native image has already been initialized.");

            // Only support color sparse image for now. Support for other aspects need to added
            if (GetAspectFlags() != RHI::ImageAspectFlags::Color)
            {
                return RHI::ResultCode::Fail;
            }

            auto& device = static_cast<Device&>(GetDevice());
            const auto& physicalDevice = static_cast<const PhysicalDevice&>(device.GetPhysicalDevice());
            const VkPhysicalDeviceFeatures& deviceFeatures = physicalDevice.GetPhysicalDeviceFeatures();
            const RHI::ImageDescriptor& descriptor = GetDescriptor();

            if (descriptor.m_dimension == RHI::ImageDimension::Image1D)
            {
                return RHI::ResultCode::Fail;
            }

            if (!(descriptor.m_multisampleState.m_samples == 1
                || (descriptor.m_multisampleState.m_samples == 2 && deviceFeatures.sparseResidency2Samples)
                || (descriptor.m_multisampleState.m_samples == 4 && deviceFeatures.sparseResidency4Samples)
                || (descriptor.m_multisampleState.m_samples == 8 && deviceFeatures.sparseResidency8Samples)
                || (descriptor.m_multisampleState.m_samples == 16 && deviceFeatures.sparseResidency16Samples))
                )
            {
                return RHI::ResultCode::Fail;
            }

            VkImageCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.format = ConvertFormat(descriptor.m_format);
            createInfo.flags = CalculateImageCreateFlags();
            // Add flags for sparse binding and sparse memory residency
            createInfo.flags |= VK_IMAGE_CREATE_SPARSE_BINDING_BIT | VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT; 
            createInfo.imageType = ConvertToImageType(descriptor.m_dimension);
            createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            createInfo.usage = CalculateImageUsageFlags();

            // Get general image properties
            VkImageFormatProperties formatProps{};
            AssertSuccess(device.GetContext().GetPhysicalDeviceImageFormatProperties(
                physicalDevice.GetNativePhysicalDevice(), createInfo.format, createInfo.imageType, createInfo.tiling, createInfo.usage,
                createInfo.flags, &formatProps));

            VkSampleCountFlagBits sampleCountFlagBits = static_cast<VkSampleCountFlagBits>(RHI::FilterBits(static_cast<VkSampleCountFlags>(ConvertSampleCount(descriptor.m_multisampleState.m_samples)), formatProps.sampleCounts));
            createInfo.samples = (static_cast<uint32_t>(sampleCountFlagBits) > 0) ? sampleCountFlagBits : VK_SAMPLE_COUNT_1_BIT;
            
            VkExtent3D extent = ConvertToExtent3D(descriptor.m_size);
            extent.width = AZStd::min<uint32_t>(extent.width, formatProps.maxExtent.width);
            extent.height = AZStd::min<uint32_t>(extent.height, formatProps.maxExtent.height);
            extent.depth = AZStd::min<uint32_t>(extent.depth, formatProps.maxExtent.depth);
            createInfo.extent = extent;
            createInfo.mipLevels = AZStd::min<uint32_t>(descriptor.m_mipLevels, formatProps.maxMipLevels);
            createInfo.arrayLayers = AZStd::min<uint32_t>(descriptor.m_arraySize, formatProps.maxArrayLayers);

            // We will always handles image's ownership
            createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 1;
            createInfo.pQueueFamilyIndices = nullptr;

            createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            const VkResult result =
                device.GetContext().CreateImage(device.GetNativeDevice(), &createInfo, VkSystemAllocator::Get(), &m_vkImage);

            if (result == VkResult::VK_SUCCESS)
            {
                // If the sparse image's alignment is not standard block size, release the image and return fail
                // We only support images with standard block size due to memory allocator restriction
                VkMemoryRequirements memoryRequirements;
                device.GetContext().GetImageMemoryRequirements(device.GetNativeDevice(), m_vkImage, &memoryRequirements);
                if (memoryRequirements.alignment != SparseImageInfo::StandardBlockSize)
                {
                    device.GetContext().DestroyImage(device.GetNativeDevice(), m_vkImage, VkSystemAllocator::Get());
                    m_vkImage = VK_NULL_HANDLE;
                    return RHI::ResultCode::Fail;
                }

                m_sparseImageInfo = AZStd::make_unique<SparseImageInfo>();
                m_sparseImageInfo->Init(device, m_vkImage, descriptor);
            }

            return ConvertResult(result);
        }

        RHI::ResultCode Image::BuildNativeImage()
        {
            AZ_Assert(m_vkImage == VK_NULL_HANDLE, "Vulkan's native image has already been initialized.");

            auto& device = static_cast<Device&>(GetDevice());
            const auto& physicalDevice = static_cast<const PhysicalDevice&>(device.GetPhysicalDevice());
            const RHI::ImageDescriptor& descriptor = GetDescriptor();
            
            VkImageCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.format = ConvertFormat(descriptor.m_format);
            createInfo.flags = CalculateImageCreateFlags();
            createInfo.imageType = ConvertToImageType(descriptor.m_dimension);
            createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            createInfo.usage = CalculateImageUsageFlags();

            m_usageFlags = createInfo.usage;

            VkImageFormatProperties formatProps{};
            AssertSuccess(device.GetContext().GetPhysicalDeviceImageFormatProperties(
                physicalDevice.GetNativePhysicalDevice(), createInfo.format, createInfo.imageType, createInfo.tiling, createInfo.usage,
                createInfo.flags, &formatProps));

            AZ_Assert(descriptor.m_sharedQueueMask != RHI::HardwareQueueClassMask::None, "Invalid shared queue mask");
            AZStd::vector<uint32_t> queueFamilies(device.GetCommandQueueContext().GetQueueFamilyIndices(descriptor.m_sharedQueueMask));

            bool exclusiveOwnership = queueFamilies.size() == 1; // Only supports one queue.
             // If it's writable, then we assume that this will be used as an ImageAttachment and all proper ownership transfers will be handled by the FrameGraph.
            exclusiveOwnership |= RHI::CheckBitsAny(descriptor.m_bindFlags, RHI::ImageBindFlags::ShaderWrite | RHI::ImageBindFlags::Color | RHI::ImageBindFlags::DepthStencil);
            exclusiveOwnership |=
                RHI::CheckBitsAny(descriptor.m_sharedQueueMask, RHI::HardwareQueueClassMask::Copy) && // Supports copy queue
                RHI::CountBitsSet(static_cast<uint32_t>(descriptor.m_sharedQueueMask)) == 2;    // And ONLY copy + another queue. This means that the
                                                                                                // copy queue can transition the resource to the correct queue
                                                                                                // after finishing copying.
            
            VkExtent3D extent = ConvertToExtent3D(descriptor.m_size);
            extent.width = AZStd::min<uint32_t>(extent.width, formatProps.maxExtent.width);
            extent.height = AZStd::min<uint32_t>(extent.height, formatProps.maxExtent.height);
            extent.depth = AZStd::min<uint32_t>(extent.depth, formatProps.maxExtent.depth);
            createInfo.extent = extent;
            createInfo.mipLevels = AZStd::min<uint32_t>(descriptor.m_mipLevels, formatProps.maxMipLevels);
            createInfo.arrayLayers = AZStd::min<uint32_t>(descriptor.m_arraySize, formatProps.maxArrayLayers);
            VkSampleCountFlagBits sampleCountFlagBits = static_cast<VkSampleCountFlagBits>(RHI::FilterBits(static_cast<VkSampleCountFlags>(ConvertSampleCount(descriptor.m_multisampleState.m_samples)), formatProps.sampleCounts));
            createInfo.samples = (static_cast<uint32_t>(sampleCountFlagBits) > 0) ? sampleCountFlagBits : VK_SAMPLE_COUNT_1_BIT;
            createInfo.sharingMode = exclusiveOwnership ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilies.size());
            createInfo.pQueueFamilyIndices = queueFamilies.empty() ? nullptr : queueFamilies.data();
            createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            const VkResult result =
                device.GetContext().CreateImage(device.GetNativeDevice(), &createInfo, VkSystemAllocator::Get(), &m_vkImage);
            AssertSuccess(result);

            return ConvertResult(result);
        }

        void Image::ReleaseAllMemory(StreamingImagePool& imagePool)
        {
            if (m_isSparse)
            {
                imagePool.DeAllocateMemoryBlocks(m_sparseImageInfo->m_mipTailHeapTiles);

                for (auto& mipInfo : m_sparseImageInfo->m_nonTailMipInfos)
                {
                    imagePool.DeAllocateMemoryBlocks(mipInfo.m_heapTiles);
                }
            }
            else
            {
                if (m_memoryView.IsValid())
                {
                    imagePool.DeAllocateMemory(m_memoryView);
                    m_memoryView = MemoryView{ };
                }
            }
            m_residentSizeInBytes = 0;
            m_highestMipLevel = RHI::Limits::Image::MipCountMax;
        }

        VkImageCreateFlags Image::CalculateImageCreateFlags() const
        {
            auto& device = static_cast<Device&>(GetDevice());
            const RHI::ImageDescriptor& descriptor = GetDescriptor();

            VkImageCreateFlags flags{};

            // Spec. of VkImageCreate:
            // "If imageType is VK_IMAGE_TYPE_2D and flags contains VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, 
            //  extent.width and extent.height must be equal and arrayLayers must be greater than or equal to 6"
            // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkImageCreateInfo.html
            if (descriptor.m_dimension == RHI::ImageDimension::Image2D &&
                descriptor.m_size.m_width == descriptor.m_size.m_height &&
                descriptor.m_arraySize >= 6)
            {
                flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            }

            // For required condition of VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT, refer the spec. of VkImageViewCreate:
            // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkImageViewCreateInfo.html
            auto& physicalDevice = static_cast<const Vulkan::PhysicalDevice&>(device.GetPhysicalDevice());
            if (physicalDevice.IsFeatureSupported(DeviceFeature::Compatible2dArrayTexture) &&
                (descriptor.m_dimension == RHI::ImageDimension::Image3D))
            {
                // The KHR value will map to the core one in case compatible 2D array is part of core.
                flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT_KHR;
            }

            if (physicalDevice.IsFeatureSupported(DeviceFeature::CustomSampleLocation) &&
                descriptor.m_multisampleState.m_customPositionsCount > 0 &&
                RHI::CheckBitsAny(descriptor.m_bindFlags, RHI::ImageBindFlags::DepthStencil))
            {
                flags |= VK_IMAGE_CREATE_SAMPLE_LOCATIONS_COMPATIBLE_DEPTH_BIT_EXT;
            }

            flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;

            return flags;
        }

        VkImageUsageFlags Image::CalculateImageUsageFlags() const
        {
            auto& device = static_cast<Device&>(GetDevice());
            const RHI::ImageBindFlags bindFlags = GetDescriptor().m_bindFlags;
            VkImageUsageFlags usageFlags{};

            if (RHI::CheckBitsAny(bindFlags, RHI::ImageBindFlags::ShaderRead))
            {
                usageFlags |= VK_IMAGE_USAGE_SAMPLED_BIT;
            }
            if (RHI::CheckBitsAny(bindFlags, RHI::ImageBindFlags::ShaderWrite))
            {
                usageFlags |= VK_IMAGE_USAGE_STORAGE_BIT;
            }
            if (RHI::CheckBitsAny(bindFlags, RHI::ImageBindFlags::Color))
            {
                usageFlags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            }
            if (RHI::CheckBitsAny(bindFlags, RHI::ImageBindFlags::DepthStencil))
            {
                usageFlags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            }
            if (RHI::CheckBitsAny(bindFlags, RHI::ImageBindFlags::CopyWrite))
            {
                usageFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            }
            if (RHI::CheckBitsAll(bindFlags, RHI::ImageBindFlags::Color | RHI::ImageBindFlags::ShaderRead) ||
                RHI::CheckBitsAll(bindFlags, RHI::ImageBindFlags::DepthStencil | RHI::ImageBindFlags::ShaderRead))
            {
                usageFlags |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
            }
            if (RHI::CheckBitsAny(bindFlags, RHI::ImageBindFlags::ShadingRate))
            {
                switch (device.GetImageShadingRateMode())
                {
                case Device::ShadingRateImageMode::DensityMap:
                {
                    usageFlags |= VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT;
                }
                break;
                case Device::ShadingRateImageMode::ImageAttachment:
                {
                    usageFlags |= VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
                }
                break;
                default:
                {
                    AZ_Error("Image", false, "Image Shading Rate mode not supported on this platform");
                }
                break;
                }
            }

            // add transfer src usage for all images since we may want them to be copyied for preview or readback
            usageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

            const VkImageUsageFlags usageMask = device.GetImageUsageFromFormat(GetDescriptor().m_format);

            auto finalFlags = usageFlags & usageMask;

            // Output a warning about desired usages are not support
            if (finalFlags != usageFlags)
            {
                AZ_Warning("Vulkan", false, "Missing usage bit flags (unsupported): %x", usageFlags & ~finalFlags);
            }

            return finalFlags;
        }
        
        void Image::SetNameInternal(const AZStd::string_view& name)
        {
            if (IsInitialized() && !name.empty())
            {
                Debug::SetNameToObject(reinterpret_cast<uint64_t>(m_vkImage), name.data(), VK_OBJECT_TYPE_IMAGE, static_cast<Device&>(GetDevice()));
            }
        }

        void Image::ReportMemoryUsage(RHI::MemoryStatisticsBuilder& builder) const
        {
            const RHI::ImageDescriptor& descriptor = GetDescriptor();

            RHI::MemoryStatistics::Image* imageStats = builder.AddImage();
            imageStats->m_name = GetName();
            imageStats->m_bindFlags = descriptor.m_bindFlags;
            imageStats->m_sizeInBytes = m_residentSizeInBytes;
            imageStats->m_minimumSizeInBytes = m_residentSizeInBytes;
        }
        
        // We don't use vkGetImageSubresourceLayout to calculate the subresource layout because we don't use linear images.
        // vkGetImageSubresourceLayout only works for linear images.
        // We always use optimal tiling since it's more efficient. We upload the content of the image using a staging buffer.
        void Image::GetSubresourceLayoutsInternal(const RHI::ImageSubresourceRange& subresourceRange, RHI::ImageSubresourceLayout* subresourceLayouts, size_t* totalSizeInBytes) const
        {
            const RHI::ImageDescriptor& imageDescriptor = GetDescriptor();
            uint32_t byteOffset = 0;
            const uint32_t offsetAligment = 4;
            for (uint16_t arraySlice = subresourceRange.m_arraySliceMin; arraySlice <= subresourceRange.m_mipSliceMax; ++arraySlice)
            {
                for (uint16_t mipSlice = subresourceRange.m_mipSliceMin; mipSlice <= subresourceRange.m_mipSliceMax; ++mipSlice)
                {
                    RHI::ImageSubresource subresource;
                    subresource.m_arraySlice = arraySlice;
                    subresource.m_mipSlice = mipSlice;
                    RHI::ImageSubresourceLayout subresourceLayout = RHI::GetImageSubresourceLayout(imageDescriptor, subresource);

                    if (subresourceLayouts)
                    {
                        const uint32_t subresourceIndex = RHI::GetImageSubresourceIndex(mipSlice, arraySlice, imageDescriptor.m_mipLevels);
                        RHI::ImageSubresourceLayout& layout = subresourceLayouts[subresourceIndex];
                        layout.m_bytesPerRow = subresourceLayout.m_bytesPerRow;
                        layout.m_bytesPerImage = subresourceLayout.m_rowCount * subresourceLayout.m_bytesPerRow;
                        layout.m_offset = byteOffset;
                        layout.m_rowCount = subresourceLayout.m_rowCount;
                        layout.m_size = subresourceLayout.m_size;
                    }

                    byteOffset = RHI::AlignUp(byteOffset + subresourceLayout.m_bytesPerImage * subresourceLayout.m_size.m_depth, offsetAligment);
                }
            }
            
            if (totalSizeInBytes)
            {
                *totalSizeInBytes = byteOffset;
            }
        }

        void Image::SetDescriptor(const RHI::ImageDescriptor& descriptor)
        {
            Base::SetDescriptor(descriptor);

            m_ownerQueue.Init(descriptor);
            m_layout.Init(descriptor);
            SetLayout(VK_IMAGE_LAYOUT_UNDEFINED);
            SetInitalQueueOwner();
        }

        bool Image::IsStreamableInternal() const
        {
            return m_isSparse && m_sparseImageInfo->m_tailStartMip > 0;
        }

        void Image::SetInitalQueueOwner()
        {
            if (!IsInitialized())
            {
                return;
            }

            auto& device = static_cast<Device&>(GetDevice());
            const RHI::ImageDescriptor& descriptor = GetDescriptor();
            for (uint32_t i = 0; i < RHI::HardwareQueueClassCount; ++i)
            {
                if (RHI::CheckBitsAny(descriptor.m_sharedQueueMask, static_cast<RHI::HardwareQueueClassMask>(AZ_BIT(i))))
                {
                    SetOwnerQueue(device.GetCommandQueueContext().GetCommandQueue(static_cast<RHI::HardwareQueueClass>(i)).GetId());
                    break;
                }
            }
        }

    }
}

