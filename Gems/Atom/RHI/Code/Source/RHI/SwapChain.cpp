/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/MemoryStatisticsBuilder.h>
#include <Atom/RHI/SwapChain.h>
#include <Atom/RHI/RHISystemInterface.h>

namespace AZ::RHI
{
    bool SwapChain::ValidateDescriptor(const SwapChainDescriptor& descriptor) const
    {
        if (Validation::IsEnabled())
        {
            const bool isValidDescriptor = descriptor.m_dimensions.m_imageWidth != 0 && descriptor.m_dimensions.m_imageHeight != 0 &&
                descriptor.m_dimensions.m_imageCount != 0;

            if (!isValidDescriptor)
            {
                AZ_Warning("SwapChain", false, "SwapChain display dimensions cannot be 0.");
                return false;
            }
        }

        return true;
    }

    ResultCode SwapChain::Init(int deviceIndex, const SwapChainDescriptor& descriptor)
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

        MultiDevice::DeviceMask deviceMask{ 1u << deviceIndex };

        SwapChainDimensions nativeDimensions = descriptor.m_dimensions;
        ResultCode resultCode = ResourcePool::Init(
            deviceMask,
            [this, &descriptor, &nativeDimensions]()
            {
                ResultCode result = ResultCode::Success;

                IterateDevices(
                    [this, &descriptor, &result, &nativeDimensions](int deviceIndex)
                    {
                        auto* device = RHISystemInterface::Get()->GetDevice(deviceIndex);

                        m_deviceObjects[deviceIndex] = Factory::Get().CreateSwapChain();

                        result = GetDeviceSwapChain(deviceIndex)->Init(*device, descriptor);
                        nativeDimensions = GetDeviceSwapChain(deviceIndex)->GetDescriptor().m_dimensions;

                        return result == ResultCode::Success;
                    });

                return result;
            });

        if (resultCode == ResultCode::Success)
        {
            m_descriptor = descriptor;
            // Overwrite descriptor dimensions with the native ones (the ones assigned by the platform) returned by InitInternal.
            // Note: dimensions of each swap chain could be different, we are taking the dimensions of the last one if there are multiple
            m_descriptor.m_dimensions = nativeDimensions;

            resultCode = InitImages();

            if (const auto& name = GetName(); !name.IsEmpty())
            {
                SetName(name);
            }
        }
        else
        {
            // Reset already initialized device-specific SwapChains and set deviceMask to 0
            m_deviceObjects.clear();
            MultiDeviceObject::Init(static_cast<MultiDevice::DeviceMask>(0u));
        }

        return resultCode;
    }

    Ptr<DeviceSwapChain> SwapChain::GetDeviceSwapChain() const
    {
        // As SwapChain is always initialized for one single device, the method returns this single item by accessing map.begin()
        return AZStd::static_pointer_cast<DeviceSwapChain>(m_deviceObjects.begin()->second);
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

                    IterateObjects<DeviceSwapChain>(
                        [this, imageIdx](auto deviceIndex, auto deviceSwapChain)
                        {
                            m_images[imageIdx]->m_deviceObjects[deviceIndex] = deviceSwapChain->GetImage(imageIdx);
                        });

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

    ResultCode SwapChain::Resize(const RHI::SwapChainDimensions& dimensions)
    {
        ResultCode resultCode = ResultCode::Success;

        ShutdownImages();

        RHI::SwapChainDimensions nativeDimensions;

        IterateObjects<DeviceSwapChain>(
            [&resultCode, &nativeDimensions, &dimensions]([[maybe_unused]]auto deviceIndex, auto deviceSwapChain)
            {
                resultCode = deviceSwapChain->Resize(dimensions);
                nativeDimensions = deviceSwapChain->GetDescriptor().m_dimensions;

                return resultCode;
            });

        if (resultCode == ResultCode::Success)
        {
            m_descriptor.m_dimensions = nativeDimensions;
            resultCode = InitImages();
        }

        return resultCode;
    }

    void SwapChain::SetVerticalSyncInterval(uint32_t verticalSyncInterval)
    {
        IterateObjects<DeviceSwapChain>([verticalSyncInterval]([[maybe_unused]] auto deviceIndex, auto deviceSwapChain)
        {
            deviceSwapChain->SetVerticalSyncInterval(verticalSyncInterval);
        });
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

    bool SwapChain::IsExclusiveFullScreenPreferred() const
    {
        auto result{ true };

        IterateObjects<DeviceSwapChain>([&result]([[maybe_unused]] auto deviceIndex, auto deviceSwapChain)
        {
            result &= deviceSwapChain->IsExclusiveFullScreenPreferred();
        });

        return result;
    }

    bool SwapChain::GetExclusiveFullScreenState() const
    {
        auto result{ true };

        IterateObjects<DeviceSwapChain>([&result]([[maybe_unused]] auto deviceIndex, auto deviceSwapChain)
        {
            result &= deviceSwapChain->GetExclusiveFullScreenState();
        });

        return result;
    }

    bool SwapChain::SetExclusiveFullScreenState(bool fullScreenState)
    {
        auto result{ true };

        IterateObjects<DeviceSwapChain>([&result, fullScreenState]([[maybe_unused]] auto deviceIndex, auto deviceSwapChain)
        {
            result &= deviceSwapChain->SetExclusiveFullScreenState(fullScreenState);
        });

        return result;
    }

    void SwapChain::ProcessRecreation()
    {
        auto recreated{ false };
        IterateObjects<DeviceSwapChain>(
            [&recreated]([[maybe_unused]] auto deviceIndex, auto deviceSwapChain)
            {
                recreated = deviceSwapChain->ProcessRecreation();
            });

        if (recreated)
        {
            ShutdownImages();
            InitImages();
        }
    }

    uint32_t SwapChain::GetImageCount() const
    {
        return aznumeric_cast<uint32_t>(m_images.size());
    }

    Image* SwapChain::GetCurrentImage() const
    {
        if (m_descriptor.m_isXrSwapChain)
        {
#if defined (AZ_FORCE_CPU_GPU_INSYNC)
            return m_images[0].get();
#else
            return m_images[m_xrSystem->GetCurrentImageIndex(m_descriptor.m_xrSwapChainIndex)].get();
#endif
        }
        AZ_Error("Swapchain", !m_deviceObjects.empty(), "No device swapchain image available.");
        // Note: Taking the current swapchain image index from the first device swap chain if there are multiple
        auto currentImageIndex{ AZStd::static_pointer_cast<DeviceSwapChain>(m_deviceObjects.begin()->second)->GetCurrentImageIndex() };
        return m_images[currentImageIndex].get();
    }

    Image* SwapChain::GetImage(uint32_t index) const
    {
        return m_images[index].get();
    }

    void SwapChain::Present()
    {
        IterateObjects<DeviceSwapChain>([]([[maybe_unused]] auto deviceIndex, auto deviceSwapChain)
        {
            deviceSwapChain->Present();
        });
    }

    RHI::XRRenderingInterface* SwapChain::GetXRSystem() const
    {
        return m_xrSystem;
    }

    void SwapChain::Shutdown()
    {
        ResourcePool::Shutdown();
    }
} // namespace AZ::RHI
