/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/Device.h>
#include <RHI/Image.h>
#include <RHI/PhysicalDevice.h>
#include <RHI/SwapChain.h>

namespace AZ
{
    namespace WebGPU
    {
        RHI::Ptr<SwapChain> SwapChain::Create()
        {
            return aznew SwapChain();
        }

        RHI::ResultCode SwapChain::InitInternal(
            RHI::Device& deviceBase,
            const RHI::SwapChainDescriptor& descriptor,
            RHI::SwapChainDimensions* nativeDimensions)
        {
            Device& device = static_cast<Device&>(deviceBase);
            m_wgpuSurface = BuildNativeSurface(descriptor);
            if (!m_wgpuSurface)
            {
                AZ_Assert(false, "Failed to build WebGPU surface");
                return RHI::ResultCode::Fail;
            }

            m_wgpuSurfaceCapabilities = GetSurfaceCapabilities(deviceBase);
            m_wgpuSurfaceFormat = GetSupportedSurfaceFormat(descriptor.m_dimensions.m_imageFormat);
            m_wgpuPresentMode = GetSupportedPresentMode(descriptor.m_verticalSyncInterval);
            m_wgpuCompositeAlphaMode = GetSupportedCompositeAlpha();

            // Configure the surface.
            wgpu::SurfaceConfiguration config = {};
            config.device = device.GetNativeDevice();
            config.format = m_wgpuSurfaceFormat;
            config.width = descriptor.m_dimensions.m_imageWidth;
            config.height = descriptor.m_dimensions.m_imageHeight;
            config.presentMode = m_wgpuPresentMode;
            config.alphaMode = m_wgpuCompositeAlphaMode;
            config.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::TextureBinding;
            m_wgpuSurface.Configure(&config);

             if (nativeDimensions)
            {
                // Fill out the real swapchain dimensions to return
                *nativeDimensions = descriptor.m_dimensions;
                nativeDimensions->m_imageFormat = ConvertImageFormat(m_wgpuSurfaceFormat);
            }

            return RHI::ResultCode::Success;
        }

        void SwapChain::ShutdownInternal()
        {
            m_wgpuSurface.Unconfigure();
            m_wgpuSurface = nullptr;
        }

        uint32_t SwapChain::PresentInternal()
        {
            AZ_Assert(m_wgpuSurface, "Invalid wgpu::Surface");
            m_wgpuSurface.Present();
            uint32_t nextImageIndex = (GetCurrentImageIndex() + 1) % GetImageCount();
            AcquireNextImage(nextImageIndex);
            return nextImageIndex;
        }

        RHI::ResultCode SwapChain::InitImageInternal(const InitImageRequest& request)
        {
            auto& device = static_cast<Device&>(GetDevice());
            Image* image = static_cast<Image*>(request.m_image);
            RHI::ImageDescriptor imageDesc = request.m_descriptor;
            RHI::ResultCode result = RHI::ResultCode::Success;

            // Initialize the swapchain image with a null texture. Will update the
            // wgpu::Texture with a proper value when acquiring a new image from
            // the Swapchain (wgpu::Surface::GetCurrentTexture)
            wgpu::Texture nullTexture = nullptr;
            imageDesc.m_format = ConvertImageFormat(m_wgpuSurfaceFormat);
            result = image->Init(device, nullTexture, imageDesc);
            
            if (result != RHI::ResultCode::Success)
            {
                AZ_Assert(false, "Failed to initialize swapchain image %d", request.m_imageIndex);
                return result;
            }

            Name name(AZStd::string::format("SwapChainImage_%d", request.m_imageIndex));
            image->SetName(name);

            if (request.m_imageIndex == GetCurrentImageIndex())
            {
                AcquireNextImage(request.m_imageIndex);
            }
            return result;
        }

        wgpu::SurfaceCapabilities SwapChain::GetSurfaceCapabilities(RHI::Device& deviceBase)
        {
            AZ_Assert(m_wgpuSurface, "Surface has not been initialized.");
            const PhysicalDevice& physicalDevice = static_cast<const PhysicalDevice&>(deviceBase.GetPhysicalDevice());
            wgpu::SurfaceCapabilities capabilities;
            auto result = m_wgpuSurface.GetCapabilities(physicalDevice.GetNativeAdapter(), &capabilities);
            AssertSuccess(result);
            return capabilities;
        }

        wgpu::TextureFormat SwapChain::GetSupportedSurfaceFormat(const RHI::Format rhiFormat) const
        {
            AZ_Assert(m_wgpuSurface, "Surface has not been initialized.");
            AZ_Assert(m_wgpuSurfaceCapabilities.formatCount > 0, "Surface capabilities not initialized");
            const wgpu::TextureFormat format = ConvertImageFormat(rhiFormat);
            for (uint32_t index = 0; index < m_wgpuSurfaceCapabilities.formatCount; ++index)
            {
                if (m_wgpuSurfaceCapabilities.formats[index] == format)
                {
                    return format;
                }
            }
            AZ_Warning("WebGPU", false, "Format %d is not supported, using %d instead.", format, m_wgpuSurfaceCapabilities.formats[0]);
            return m_wgpuSurfaceCapabilities.formats[0];
        }

        wgpu::PresentMode SwapChain::GetSupportedPresentMode(uint32_t verticalSyncInterval) const
        {
            AZ_Assert(m_wgpuSurface, "Surface has not been initialized.");
            AZ_Assert(m_wgpuSurfaceCapabilities.presentModeCount > 0, "Surface capabilities not initialized");
            AZStd::vector<wgpu::PresentMode> preferredModes;
            if (verticalSyncInterval == 0)
            {
                // No vsync, so we try to use the immediate mode.
                preferredModes.push_back(wgpu::PresentMode::Immediate);
                // If immediate mode is not available we try for mailbox, which technically is still vsync
                // but doesn't block the CPU when queue is full.
                preferredModes.push_back(wgpu::PresentMode::Mailbox);
            }

            preferredModes.push_back(wgpu::PresentMode::Fifo);
            for (const auto& preferredMode : preferredModes)
            {
                for (uint32_t i = 0; i < m_wgpuSurfaceCapabilities.presentModeCount; ++i)
                {
                    if (m_wgpuSurfaceCapabilities.presentModes[i] == preferredMode)
                    {
                        return preferredMode;
                    }
                }
            }
            return m_wgpuSurfaceCapabilities.presentModes[0];
        }

        wgpu::CompositeAlphaMode SwapChain::GetSupportedCompositeAlpha() const
        {
            AZ_Assert(m_wgpuSurface, "Surface has not been initialized.");
            AZ_Assert(m_wgpuSurfaceCapabilities.alphaModeCount > 0, "Surface capabilities not initialized");

            wgpu::CompositeAlphaMode preferedModes[] = { wgpu::CompositeAlphaMode::Opaque,
                                                         wgpu::CompositeAlphaMode::Inherit,
                                                         wgpu::CompositeAlphaMode::Premultiplied,
                                                         wgpu::CompositeAlphaMode::Unpremultiplied };

            for (const auto& mode : preferedModes)
            {
                for (uint32_t i = 0; i < m_wgpuSurfaceCapabilities.alphaModeCount; ++i)
                {
                    if (m_wgpuSurfaceCapabilities.alphaModes[i] == mode)
                    {
                        return mode;
                    }
                }
            }

            AZ_Assert(false, "Could not find a supported composite alpha mode for the swapchain");
            return m_wgpuSurfaceCapabilities.alphaModes[0];
        }

        void SwapChain::AcquireNextImage(uint32_t imageIndex)
        {
            wgpu::SurfaceTexture surfaceTexture = {};
            m_wgpuSurface.GetCurrentTexture(&surfaceTexture);
            AZ_Assert(
                surfaceTexture.status == wgpu::SurfaceGetCurrentTextureStatus::Success,
                "Failed to get the current texture %d",
                surfaceTexture.status);

            Image* image = static_cast<Image*>(GetImage(imageIndex));
            image->SetNativeTexture(surfaceTexture.texture);
        }
    }
}
