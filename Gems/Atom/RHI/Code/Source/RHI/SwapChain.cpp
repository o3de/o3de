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
#include <Atom/RHI/RHISystemInterface.h>
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

        ResultCode SwapChain::Init(RHI::DeviceMask deviceMask, const SwapChainDescriptor& descriptor)
        {
            if (!ValidateDescriptor(descriptor))
            {
                return ResultCode::InvalidArgument;
            }

            if (descriptor.m_isXrSwapChain)
            {
                m_xrSystem = RHI::RHISystemInterface::Get()->GetXRSystem();
                AZ_Assert(m_xrSystem, "XR System is null");
            }

            SwapChainDimensions nativeDimensions = descriptor.m_dimensions;
            ResultCode resultCode = ResourcePool::Init(
                deviceMask,
                descriptor,
                [this, &descriptor, &nativeDimensions]()
                {
                    ResultCode result = ResultCode::Success;

                    IterateDevices(
                        [this, &descriptor, &result, &nativeDimensions](int deviceIndex)
                        {
                            auto* device = RHISystemInterface::Get()->GetDevice(deviceIndex);

                            m_deviceSwapChains[deviceIndex] = Factory::Get().CreateSwapChain();
                            result = m_deviceSwapChains[deviceIndex]->Init(*device, descriptor);
                            nativeDimensions = m_deviceSwapChains[deviceIndex]->GetDescriptor().m_dimensions;

                            return result == ResultCode::Success;
                        });

                    return result;
                });

            if (resultCode == ResultCode::Success)
            {
                m_descriptor = descriptor;
                // Overwrite descriptor dimensions with the native ones (the ones assigned by the platform) returned by InitInternal.
                // TODO: dimensions of each swap chain could be different
                m_descriptor.m_dimensions = nativeDimensions;

                InitImages();
            }

            return resultCode;
        }

        void SwapChain::ShutdownImages()
        {
            // Shutdown existing set of images.
            uint32_t imageSize = aznumeric_cast<uint32_t>(m_images.size());
            for (uint32_t imageIdx = 0; imageIdx < imageSize; ++imageIdx)
            {
                m_images[imageIdx]->Shutdown();
            }

            m_images.clear();
        }

        ResultCode SwapChain::InitImages()
        {
            ResultCode resultCode = ResultCode::Success;

            m_images.reserve(m_descriptor.m_dimensions.m_imageCount);

            // If the new display mode has more buffers, add them.
            for (uint32_t i = 0; i < m_descriptor.m_dimensions.m_imageCount; ++i)
            {
                m_images.emplace_back(aznew Image());
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
                    [this, imageIdx]()
                    {
                        ResultCode result = ResultCode::Success;

                        for (auto& [deviceIndex, deviceSwapChain] : m_deviceSwapChains)
                        {
                            m_images[imageIdx]->m_deviceImages[deviceIndex] = deviceSwapChain->GetImage(imageIdx);
                        }

                        return result;
                    });

                if (resultCode != ResultCode::Success)
                {
                    AZ_Error("Swapchain", false, "Failed to initialize images.");
                    Shutdown();
                    break;
                }
            }

            return resultCode;
        }

        void SwapChain::ShutdownInternal()
        {
            m_images.clear();
            for (auto& [deviceIndex, deviceSwapChain] : m_deviceSwapChains)
            {
                deviceSwapChain->Shutdown();
            }

            ResourcePool::ShutdownInternal();
        }

        ResultCode SwapChain::Resize(const RHI::SwapChainDimensions& dimensions)
        {
            ResultCode resultCode = ResultCode::Success;

            ShutdownImages();

            for (auto& [deviceIndex, deviceSwapChain] : m_deviceSwapChains)
            {
                resultCode = deviceSwapChain->Resize(dimensions);

                if (resultCode != ResultCode::Success)
                    break;
            }

            if (resultCode == ResultCode::Success)
            {
                m_descriptor.m_dimensions = dimensions;
                InitImages();
            }

            return resultCode;
        }

        void SwapChain::SetVerticalSyncInterval(uint32_t verticalSyncInterval)
        {
            for (auto& [deviceIndex, deviceSwapChain] : m_deviceSwapChains)
            {
                deviceSwapChain->SetVerticalSyncInterval(verticalSyncInterval);
            }

            m_descriptor.m_verticalSyncInterval = verticalSyncInterval;
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
            return aznumeric_cast<uint32_t>(m_images.size());
        }

        Image* SwapChain::GetCurrentImage() const
        {
            if (m_descriptor.m_isXrSwapChain)
            {
                return m_images[m_xrSystem->GetCurrentImageIndex(m_descriptor.m_xrSwapChainIndex)].get();
            }
            // TODO: Take the current swapchain image index from the first device swap chain
            AZ_Error("Swapchain", !m_deviceSwapChains.empty(), "No device swapchain image available.");
            auto currentImageIndex{ m_deviceSwapChains.begin()->second->GetCurrentImageIndex() };
            return m_images[currentImageIndex].get();
        }

        Image* SwapChain::GetImage(uint32_t index) const
        {
            return m_images[index].get();
        }

        void SwapChain::Present()
        {
            for (auto& [deviceIndex, deviceSwapChain] : m_deviceSwapChains)
            {
                deviceSwapChain->Present();
            }
        }
    }
}
