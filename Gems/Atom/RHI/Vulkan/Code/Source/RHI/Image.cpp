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
#include <RHI/Image.h>
#include <RHI/ImagePool.h>
#include <RHI/SwapChain.h>
#include <RHI/Fence.h>
#include <RHI/PhysicalDevice.h>
#include <RHI/MemoryAllocator.h>
#include <RHI/Device.h>
#include <RHI/ImageView.h>
#include <RHI/Queue.h>
#include <RHI/ReleaseContainer.h>

namespace AZ
{
    namespace Vulkan
    {
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
            }
            m_vkImage = VK_NULL_HANDLE;
            m_memoryView = MemoryView();
            m_layout.Reset();
            m_ownerQueue.Reset();
        }

        const MemoryView* Image::GetMemoryView() const
        {
            return &m_memoryView;
        }

        MemoryView* Image::GetMemoryView()
        {
            return &m_memoryView;
        }

        VkImage Image::GetNativeImage() const
        {
            return m_vkImage;
        }

        bool Image::IsOwnerOfNativeImage() const
        {
            return m_isOwnerOfNativeImage;
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

        RHI::ResultCode Image::Init(Device& device, const RHI::ImageDescriptor& descriptor)
        {
            RHI::DeviceObject::Init(device);
            SetDescriptor(descriptor);
            RHI::ResultCode result = BuildNativeImage();
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            device.GetContext().GetImageMemoryRequirements(device.GetNativeDevice(), m_vkImage, &m_memoryRequirements);
            SetName(GetName());
            return result;
        }

        RHI::ResultCode Image::BindMemoryView(const MemoryView& memoryView, [[maybe_unused]] const RHI::HeapMemoryLevel heapMemoryLevel)
        {
            AZ_Assert(m_vkImage != VK_NULL_HANDLE, "Vulkan's native image is not initialized.");
            AZ_Assert(memoryView.IsValid(), "MemoryView is not valid.");

            auto& device = static_cast<Device&>(GetDevice());
            VkResult vkResult = device.GetContext().BindImageMemory(
                device.GetNativeDevice(), m_vkImage, memoryView.GetMemory()->GetNativeDeviceMemory(), memoryView.GetOffset());
            AssertSuccess(vkResult);
            RHI::ResultCode result = ConvertResult(vkResult);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);
            m_memoryView = memoryView;
            return result;
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
            createInfo.flags = GetImageCreateFlags();
            createInfo.imageType = ConvertToImageType(descriptor.m_dimension);
            createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            createInfo.usage = GetImageUsageFlags();

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

            const VkResult result = device.GetContext().CreateImage(device.GetNativeDevice(), &createInfo, nullptr, &m_vkImage);
            AssertSuccess(result);

            m_isOwnerOfNativeImage = true;
            return ConvertResult(result);
        }

        VkImageCreateFlags Image::GetImageCreateFlags() const
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

            flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;

            return flags;
        }

        VkImageUsageFlags Image::GetImageUsageFlags() const
        {
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

            // add transfer src usage for all images since we may want them to be copyied for preview or readback
            usageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

            auto& device = static_cast<Device&>(GetDevice());
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
        }
        
        // We don't use vkGetImageSubresourceLayout to calculate the subresource layout because we don't use linear images.
        // vkGetImageSubresourceLayout only works for linear images.
        // We always use optimal tiling since it's more efficient. We upload the content of the image using a staging buffer.
        void Image::GetSubresourceLayoutsInternal(const RHI::ImageSubresourceRange& subresourceRange, RHI::ImageSubresourceLayoutPlaced* subresourceLayouts, size_t* totalSizeInBytes) const
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
                        RHI::ImageSubresourceLayoutPlaced& layout = subresourceLayouts[subresourceIndex];
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
