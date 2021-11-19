/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/SwapChain.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/MemoryStatisticsBuilder.h>
#include <AzCore/Debug/EventTrace.h>

namespace AZ
{
    namespace RHI
    {
        SwapChain::SwapChain() {}

        SwapChain::~SwapChain() {}

        bool SwapChain::ValidateDescriptor(const SwapChainDescriptor& descriptor) const
        {
            if (Validation::IsEnabled())
            {
                const bool isValidDescriptor =
                    descriptor.m_dimensions.m_imageWidth != 0 &&
                    descriptor.m_dimensions.m_imageHeight != 0 &&
                    descriptor.m_dimensions.m_imageCount != 0;

                if (!isValidDescriptor)
                {
                    AZ_Warning("SwapChain", false, "SwapChain display dimensions cannot be 0.");
                    return false;
                }
            }

            return true;
        }

        ResultCode SwapChain::Init(RHI::Device& device, const SwapChainDescriptor& descriptor)
        {
            if (!ValidateDescriptor(descriptor))
            {
                return ResultCode::InvalidArgument;
            }

            SwapChainDimensions nativeDimensions = descriptor.m_dimensions;
            ResultCode resultCode = ResourcePool::Init(
                device, descriptor,
                [this, &device, &descriptor, &nativeDimensions]()
            {
                return InitInternal(device, descriptor, &nativeDimensions);
            });

            if (resultCode == ResultCode::Success)
            {
                m_descriptor = descriptor;
                // Overwrite descriptor dimensions with the native ones (the ones assigned by the platform) returned by InitInternal.
                m_descriptor.m_dimensions = nativeDimensions;

                m_images.reserve(m_descriptor.m_dimensions.m_imageCount);

                for (uint32_t imageIdx = 0; imageIdx < m_descriptor.m_dimensions.m_imageCount; ++imageIdx)
                {
                    m_images.emplace_back(RHI::Factory::Get().CreateImage());
                }

                InitImageRequest request;

                RHI::ImageDescriptor& imageDescriptor = request.m_descriptor;
                imageDescriptor.m_dimension = RHI::ImageDimension::Image2D;
                imageDescriptor.m_bindFlags = RHI::ImageBindFlags::Color;
                imageDescriptor.m_size.m_width = m_descriptor.m_dimensions.m_imageWidth;
                imageDescriptor.m_size.m_height = m_descriptor.m_dimensions.m_imageHeight;
                imageDescriptor.m_format = m_descriptor.m_dimensions.m_imageFormat;

                for (uint32_t imageIdx = 0; imageIdx < m_descriptor.m_dimensions.m_imageCount; ++imageIdx)
                {
                    request.m_image = m_images[imageIdx].get();
                    request.m_imageIndex = imageIdx;

                    resultCode = ImagePoolBase::InitImage(
                        request.m_image,
                        imageDescriptor,
                        [this, &request]()
                    {
                        return InitImageInternal(request);
                    });

                    if (resultCode != ResultCode::Success)
                    {
                        Shutdown();
                        break;
                    }
                }
            }

            return resultCode;
        }

        void SwapChain::ShutdownInternal()
        {
            m_images.clear();
            ResourcePool::ShutdownInternal();
        }

        ResultCode SwapChain::Resize(const RHI::SwapChainDimensions& dimensions)
        {     
            // Shutdown existing set of images.
            for (uint32_t imageIdx = 0; imageIdx < GetImageCount(); ++imageIdx)
            {
                m_images[imageIdx]->Shutdown();
            }

            SwapChainDimensions nativeDimensions = dimensions;
            ResultCode resultCode = ResizeInternal(dimensions, &nativeDimensions);
            if (resultCode == ResultCode::Success)
            {
                m_descriptor.m_dimensions = nativeDimensions;
                m_images.reserve(m_descriptor.m_dimensions.m_imageCount);

                // If the new display mode has more buffers, add them.
                while (m_images.size() < static_cast<size_t>(m_descriptor.m_dimensions.m_imageCount))
                {
                    m_images.emplace_back(RHI::Factory::Get().CreateImage());
                }

                // If it has fewer, trim down.
                while (m_images.size() > static_cast<size_t>(m_descriptor.m_dimensions.m_imageCount))
                {
                    m_images.pop_back();
                }

                InitImageRequest request;

                RHI::ImageDescriptor& imageDescriptor = request.m_descriptor;
                imageDescriptor.m_dimension = RHI::ImageDimension::Image2D;
                imageDescriptor.m_bindFlags = RHI::ImageBindFlags::Color;
                imageDescriptor.m_size.m_width = m_descriptor.m_dimensions.m_imageWidth;
                imageDescriptor.m_size.m_height = m_descriptor.m_dimensions.m_imageHeight;
                imageDescriptor.m_format = m_descriptor.m_dimensions.m_imageFormat;

                for (uint32_t imageIdx = 0; imageIdx < GetImageCount(); ++imageIdx)
                {
                    request.m_image = m_images[imageIdx].get();
                    request.m_imageIndex = imageIdx;

                    resultCode = ImagePoolBase::InitImage(
                        request.m_image,
                        imageDescriptor,
                        [this, &request]()
                    {
                        return InitImageInternal(request);
                    });

                    if (resultCode != ResultCode::Success)
                    {
                        Shutdown();
                        break;
                    }
                }

                // Reset the current index back to 0 so we match the platform swap chain.
                m_currentImageIndex = 0;
            }

            return resultCode;
        }

        void SwapChain::SetVerticalSyncInterval(uint32_t verticalSyncInterval)
        {
            uint32_t previousVsyncInterval = m_descriptor.m_verticalSyncInterval;

            m_descriptor.m_verticalSyncInterval = verticalSyncInterval;

            SetVerticalSyncIntervalInternal(previousVsyncInterval);
        }

        const AttachmentId& SwapChain::GetAttachmentId() const
        {
            return m_descriptor.m_attachmentId;
        }

        const SwapChainDescriptor& SwapChain::GetDescriptor() const
        {
            return m_descriptor;
        }

        uint32_t SwapChain::GetImageCount() const
        {
            return static_cast<uint32_t>(m_images.size());
        }

        uint32_t SwapChain::GetCurrentImageIndex() const
        {
            return m_currentImageIndex;
        }

        Image* SwapChain::GetCurrentImage() const
        {
            return m_images[m_currentImageIndex].get();
        }

        Image* SwapChain::GetImage(uint32_t index) const
        {
            return m_images[index].get();
        }

        void SwapChain::Present()
        {
            AZ_TRACE_METHOD();
            m_currentImageIndex = PresentInternal();
            AZ_Assert(m_currentImageIndex < m_images.size(), "Invalid image index");
        }
    }
}
