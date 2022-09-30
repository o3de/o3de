/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <RHI/Device.h>
#include <RHI/Image.h>
#include <RHI/ImageView.h>
#include <RHI/ReleaseContainer.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<ImageView> ImageView::Create()
        {
            return aznew ImageView();
        }

        VkImageView ImageView::GetNativeImageView() const
        {
            return m_vkImageView;
        }

        RHI::Format ImageView::GetFormat() const
        {
            return m_format;
        }

        const RHI::ImageSubresourceRange& ImageView::GetImageSubresourceRange() const
        {
            return m_imageSubresourceRange;
        }

        void ImageView::SetNameInternal(const AZStd::string_view& name)
        {
            if (IsInitialized() && !name.empty())
            {
                Debug::SetNameToObject(reinterpret_cast<uint64_t>(m_vkImageView), name.data(), VK_OBJECT_TYPE_IMAGE_VIEW, static_cast<Device&>(GetDevice()));
            }
        }

        RHI::ResultCode ImageView::InitInternal(RHI::Device& deviceBase, const RHI::Resource& resourceBase)
        {
            DeviceObject::Init(deviceBase);
            auto& device = static_cast<Device&>(deviceBase);
            const auto& image = static_cast<const Image&>(resourceBase);
            const RHI::ImageViewDescriptor& descriptor = GetDescriptor();

            // this can happen when image has been invalidated/released right before re-compiling the image
            if (image.GetNativeImage() == VK_NULL_HANDLE)
            {
                return RHI::ResultCode::Fail;
            }

            RHI::Format viewFormat = descriptor.m_overrideFormat;
            // If an image is not owner of native image, it is a swapchain image.
            // Swapchain images are not mutable, so we can not change the format for the view.
            if (viewFormat == RHI::Format::Unknown || !image.IsOwnerOfNativeImage())
            {
                viewFormat = image.GetDescriptor().m_format;
            }
            m_format = viewFormat;

            VkImageAspectFlags aspectFlags = ConvertImageAspectFlags(RHI::FilterBits(descriptor.m_aspectFlags, image.GetAspectFlags()));
            
            const VkImageViewType imageViewType = GetImageViewType(image);
            BuildImageSubresourceRange(imageViewType, aspectFlags);
            const RHI::ImageSubresourceRange& range = GetImageSubresourceRange();
            VkImageSubresourceRange vkRange{};
            vkRange.baseMipLevel = range.m_mipSliceMin;
            vkRange.levelCount = range.m_mipSliceMax - range.m_mipSliceMin + 1;
            vkRange.baseArrayLayer = range.m_arraySliceMin;
            vkRange.layerCount = range.m_arraySliceMax - range.m_arraySliceMin + 1;
            vkRange.aspectMask = aspectFlags;
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.flags = 0;
            createInfo.image = image.GetNativeImage();
            createInfo.viewType = imageViewType;
            createInfo.format = ConvertFormat(m_format);
            createInfo.components = VkComponentMapping{}; // identity mapping
            createInfo.subresourceRange = vkRange;

            const VkResult result = device.GetContext().CreateImageView(device.GetNativeDevice(), &createInfo, nullptr, &m_vkImageView);
            AssertSuccess(result);
            RETURN_RESULT_IF_UNSUCCESSFUL(ConvertResult(result));

            m_hash = TypeHash64(m_imageSubresourceRange.GetHash(), m_hash);
            m_hash = TypeHash64(m_format, m_hash);

            SetName(GetName());
            return RHI::ResultCode::Success;
        }

        RHI::ResultCode ImageView::InvalidateInternal()
        {
            ShutdownInternal();
            return InitInternal(GetDevice(), GetResource());
        }

        void ImageView::ShutdownInternal()
        {            
            if (m_vkImageView != VK_NULL_HANDLE)
            {
                auto& device = static_cast<Device&>(GetDevice());
                device.QueueForRelease(
                    new ReleaseContainer<VkImageView>(device.GetNativeDevice(), m_vkImageView, device.GetContext().DestroyImageView));
                m_vkImageView = VK_NULL_HANDLE;
            }
        }

        VkImageViewType ImageView::GetImageViewType(const Image& image) const
        {
            const RHI::ImageDescriptor& imgDesc = image.GetDescriptor();
            const RHI::ImageViewDescriptor& imgViewDesc = GetDescriptor();
            const auto& device = static_cast<const Device&>(GetDevice());
            const auto& physicalDevice = static_cast<const PhysicalDevice&>(GetDevice().GetPhysicalDevice());

            [[maybe_unused]] const uint16_t width = static_cast<uint16_t>(imgDesc.m_size.m_width);
            [[maybe_unused]] const uint16_t height = static_cast<uint16_t>(imgDesc.m_size.m_height);
            const uint16_t depth = AZStd::min(static_cast<uint16_t>(imgViewDesc.m_depthSliceMax - imgViewDesc.m_depthSliceMin), static_cast<uint16_t>(imgDesc.m_size.m_depth - 1)) + 1;
            [[maybe_unused]] const uint16_t samples = imgDesc.m_multisampleState.m_samples;
            const uint16_t arrayLayers = AZStd::min(static_cast<uint16_t>(imgViewDesc.m_arraySliceMax - imgViewDesc.m_arraySliceMin), static_cast<uint16_t>(imgDesc.m_arraySize - 1)) + 1;
            // We cannot only use the number of layers of the ImageView to determinate if is a texture array. You can have a texture array with only 1 layer and the shader expects
            // an array type instead of a normal image type.
            const bool isArray = arrayLayers > 1 || imgViewDesc.m_isArray;
            AZ_Assert(width * height * depth * samples * arrayLayers != 0, "One of width, height, depth, samples, or arrayLayers = 0.");

            switch (imgDesc.m_dimension)
            {
            case RHI::ImageDimension::Image1D:
            {
                AZ_Assert(height == 1 && depth == 1, "Both height and depth must be 1 for Image1D or Image1DArray.");
                return isArray ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
            case RHI::ImageDimension::Image2D:
                AZ_Assert(depth == 1, "Depth must be 1 for Image2D or Image2DArray.");
                if (!isArray)
                {
                    return VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
                }
                else
                {
                    if (imgViewDesc.m_isCubemap)
                    {
                        AZ_Assert(width == height, "Image of Cube or CubeArray form a square.");
                        AZ_Assert(depth == 1, "Depth of Cube or CubeArray = 1.");
                        AZ_Assert(samples == 1, "Sample of Cube or CubeArray = 1.");
                        AZ_Assert(arrayLayers % 6 == 0, "ArrayLayers %% 6 == 0 for Cube or CubeArray.");
                        if (arrayLayers == 6)
                        {
                            return VK_IMAGE_VIEW_TYPE_CUBE;
                        }
                        else
                        {
                            if (device.GetEnabledDevicesFeatures().imageCubeArray)
                            {
                                return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
                            }
                            else
                            {
                                AZ_Assert(false, "This device does not support Image Cube Array.");
                                return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
                            }
                        }
                    }
                    else
                    {
                        return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
                    }
                }
            }
            case RHI::ImageDimension::Image3D:
            {
                // Vulkan does not have capability to give a view of proper subset of depth slices of a base 3D Image 
                // as an ImageView of type 3D, unlike Direct3D.  (We can still specify a proper subset of mip slices.)
                // Instead of it, we can get it as an ImageView of type 2D or 2D array for a proper depth slice subset.
                // One of the main usage of depth slices of a 3D Image would be to set the slice to a Framebuffer,
                // and here we give an image view of 2D or 2D array.
                AZ_Assert(arrayLayers == 1 && samples == 1, "Both of arrayLayers and samples must be 1 for Image3D.");
                if (physicalDevice.IsFeatureSupported(DeviceFeature::Compatible2dArrayTexture))
                {
                    if (imgViewDesc.m_depthSliceMin == imgViewDesc.m_depthSliceMax)
                    {
                        return VK_IMAGE_VIEW_TYPE_2D;
                    }
                    else if (imgViewDesc.m_depthSliceMin == 0 && imgViewDesc.m_depthSliceMax + 1 >= depth)
                    {
                        // If ImageView's depth slice range covers the one of the base 3D Image,
                        // it returns the view of the entire depth slices.
                        return VK_IMAGE_VIEW_TYPE_3D;
                    }
                    else
                    {
                        return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
                    }
                }
                else
                {
                    return VK_IMAGE_VIEW_TYPE_3D;
                }
            }
            default:
            {
                AZ_Assert(false, "Image Dimension is illegal.");
                return VK_IMAGE_VIEW_TYPE_2D;
            }
            }
        }

        void ImageView::BuildImageSubresourceRange(VkImageViewType imageViewType, VkImageAspectFlags aspectFlags)
        {
            const Image& image = static_cast<const Image&>(GetImage());
            const RHI::ImageDescriptor& imageDesc = image.GetDescriptor();
            const RHI::ImageViewDescriptor& descriptor = GetDescriptor();

            RHI::ImageSubresourceRange& range = m_imageSubresourceRange;
            range.m_mipSliceMin = AZStd::max(descriptor.m_mipSliceMin, image.GetStreamedMipLevel());
            range.m_mipSliceMax = AZStd::min(descriptor.m_mipSliceMax, static_cast<uint16_t>(imageDesc.m_mipLevels - 1));
            if (imageDesc.m_dimension != RHI::ImageDimension::Image3D)
            {
                range.m_arraySliceMin = descriptor.m_arraySliceMin;
                range.m_arraySliceMax = AZStd::min(descriptor.m_arraySliceMax, static_cast<uint16_t>(imageDesc.m_arraySize - 1));
            }
            else
            {
                switch (imageViewType)
                {
                case VK_IMAGE_VIEW_TYPE_2D:
                case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
                    // To create 2D or 2D array image view of 3D image, we specify its depth slice range by
                    // baseArrayLayer and layerCount of VkImageSubresourceRange.
                    // https://www.khronos.org/registry/vulkan/specs/1.1/html/chap11.html#VkImageSubresourceRange
                {
                    range.m_arraySliceMin = descriptor.m_depthSliceMin;
                    range.m_arraySliceMax = AZStd::GetMin<uint16_t>(descriptor.m_depthSliceMax, static_cast<uint16_t>(imageDesc.m_size.m_depth - 1));
                    break;
                }
                case VK_IMAGE_VIEW_TYPE_3D:
                {
                    range.m_arraySliceMin = 0;
                    range.m_arraySliceMax = 0;
                    break;
                }
                default:
                    AZ_Assert(false, "Invalid image view type.");
                    break;
                }
            }

            m_vkImageSubResourceRange.aspectMask = aspectFlags;
            m_vkImageSubResourceRange.baseArrayLayer = range.m_arraySliceMin;
            m_vkImageSubResourceRange.layerCount = range.m_arraySliceMax - range.m_arraySliceMin + 1;
            m_vkImageSubResourceRange.baseMipLevel = range.m_mipSliceMin;
            m_vkImageSubResourceRange.levelCount = range.m_mipSliceMax - range.m_mipSliceMin + 1;            
        }

        const VkImageSubresourceRange& ImageView::GetVkImageSubresourceRange() const
        {
            return m_vkImageSubResourceRange;
        }
    }
}
