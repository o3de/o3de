/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/WindowContext.h>
#include <Atom/RPI.Public/WindowContextBus.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/Specific/SwapChainPass.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHISystemInterface.h>

#include <AzCore/Console/IConsole.h>
#include <AzCore/Math/MathUtils.h>

namespace AZ
{
    namespace RPI
    {
        void WindowContext::Initialize(RHI::Device& device, AzFramework::NativeWindowHandle windowHandle)
        {
            m_windowHandle = windowHandle;

            if (RHI::CheckBitsAll(device.GetFeatures().m_swapchainScalingFlags, RHI::ScalingFlags::AspectRatioStretch))
            {
                m_swapChainScalingMode = RHI::Scaling::AspectRatioStretch;
            }
            else if (RHI::CheckBitsAll(device.GetFeatures().m_swapchainScalingFlags, RHI::ScalingFlags::Stretch))
            {
                m_swapChainScalingMode = RHI::Scaling::Stretch;
            }
            else
            {
                m_swapChainScalingMode = RHI::Scaling::None;
            }

            CreateSwapChains(device);

            AzFramework::WindowNotificationBus::Handler::BusConnect(m_windowHandle);
            AzFramework::ExclusiveFullScreenRequestBus::Handler::BusConnect(m_windowHandle);
        }

        AZStd::vector<ViewportContextPtr> WindowContext::GetAssociatedViewportContexts()
        {
            AZStd::vector<ViewportContextPtr> associatedContexts;
            for (auto viewportContextWeakRef : m_viewportContexts)
            {
                if (auto viewportContextRef = viewportContextWeakRef.lock())
                {
                    associatedContexts.push_back(viewportContextRef);
                }
            }
            return associatedContexts;
        }

        void WindowContext::RegisterAssociatedViewportContext(ViewportContextPtr viewportContext)
        {
            m_viewportContexts.push_back(viewportContext);
        }

        void WindowContext::Shutdown()
        {
            AzFramework::ExclusiveFullScreenRequestBus::Handler::BusDisconnect(m_windowHandle);
            AzFramework::WindowNotificationBus::Handler::BusDisconnect(m_windowHandle);

            DestroyDefaultSwapChain();
            DestroyXRSwapChains();

            m_swapChainsData.clear();
        }

        const RHI::AttachmentId& WindowContext::GetSwapChainAttachmentId(ViewType viewType) const
        {   
            return GetSwapChain(viewType)->GetAttachmentId();
        }

        const RHI::Ptr<RHI::SwapChain>& WindowContext::GetSwapChain(ViewType viewType) const
        {
            uint32_t swapChainIndex = static_cast<uint32_t>(viewType);
            AZ_Assert(swapChainIndex < GetSwapChainsSize(), "Swapchain with index %i does not exist", swapChainIndex);
            return m_swapChainsData[swapChainIndex].m_swapChain;
        }

        uint32_t WindowContext::GetSwapChainsSize() const
        {
            return aznumeric_cast<uint32_t>(m_swapChainsData.size());
        }

        RHI::Scaling WindowContext::GetSwapChainScalingMode() const
        {
            return m_swapChainScalingMode;
        }

        const RHI::Viewport& WindowContext::GetViewport(ViewType viewType) const
        {
            uint32_t swapChainIndex = static_cast<uint32_t>(viewType);
            AZ_Assert(swapChainIndex < GetSwapChainsSize(), "Swapchain does not exist");
            return m_swapChainsData[swapChainIndex].m_viewport;     
        }

        const RHI::Scissor& WindowContext::GetScissor(ViewType viewType) const
        {
            uint32_t swapChainIndex = static_cast<uint32_t>(viewType);
            AZ_Assert(swapChainIndex < GetSwapChainsSize(), "Swapchain does not exist");
            return m_swapChainsData[swapChainIndex].m_scissor;  
        }
        
        void WindowContext::OnWindowResized([[maybe_unused]]uint32_t width, [[maybe_unused]]uint32_t height)
        {
            CheckResizeSwapChain();
        }

        void WindowContext::OnResolutionChanged([[maybe_unused]]uint32_t width, [[maybe_unused]]uint32_t height)
        {
            CheckResizeSwapChain();
        }

        bool WindowContext::CheckResizeSwapChain()
        {
            RHI::Ptr<RHI::SwapChain> defaultSwapChain = GetSwapChain(ViewType::Default);
            const AZ::RHI::SwapChainDimensions& currentDimensions = defaultSwapChain->GetDescriptor().m_dimensions;
            AzFramework::WindowSize renderSize = ResolveSwapchainSize();
            if (renderSize.m_width != currentDimensions.m_imageWidth || renderSize.m_height != currentDimensions.m_imageHeight)
            {
                // Get current dimension and only overwrite the sizes.
                RHI::SwapChainDimensions dimensions = defaultSwapChain->GetDescriptor().m_dimensions;
                dimensions.m_imageWidth = renderSize.m_width;
                dimensions.m_imageHeight = renderSize.m_height;
                // Note: there is only one bit set in the mask, so we simply get the device index with log2
                dimensions.m_imageFormat = GetSwapChainFormat(*RHI::RHISystemInterface::Get()->GetDevice(static_cast<int>(log2(static_cast<float>(defaultSwapChain->GetDeviceMask())))));

                FillWindowState(dimensions.m_imageWidth, dimensions.m_imageHeight);

                defaultSwapChain->Resize(dimensions);

                WindowContextNotificationBus::Event(m_windowHandle, &WindowContextNotifications::OnViewportResized, dimensions.m_imageWidth, dimensions.m_imageHeight);
                return true;
            }
            return false;
        }

        void WindowContext::OnWindowClosed()
        {
            DestroyDefaultSwapChain();
            DestroyXRSwapChains();

            // We don't want to listen to events anymore if the window has closed
            AzFramework::ExclusiveFullScreenRequestBus::Handler::BusDisconnect(m_windowHandle);
            AzFramework::WindowNotificationBus::Handler::BusDisconnect(m_windowHandle);
        }

        void WindowContext::OnVsyncIntervalChanged(uint32_t interval)
        {
            RHI::Ptr<RHI::SwapChain> defaultSwapChain = GetSwapChain(ViewType::Default);
            if (defaultSwapChain->GetDescriptor().m_verticalSyncInterval != interval)
            {
                defaultSwapChain->SetVerticalSyncInterval(interval);
            }
        }

        bool WindowContext::IsExclusiveFullScreenPreferred() const
        {
            RHI::Ptr<RHI::SwapChain> defaultSwapChain = GetSwapChain(ViewType::Default);
            return defaultSwapChain->IsExclusiveFullScreenPreferred();
        }

        bool WindowContext::GetExclusiveFullScreenState() const
        {
            RHI::Ptr<RHI::SwapChain> defaultSwapChain = GetSwapChain(ViewType::Default);
            return defaultSwapChain->GetExclusiveFullScreenState();
        }

        bool WindowContext::SetExclusiveFullScreenState(bool fullScreenState)
        {
            RHI::Ptr<RHI::SwapChain> defaultSwapChain = GetSwapChain(ViewType::Default);
            return defaultSwapChain->SetExclusiveFullScreenState(fullScreenState);
        }

        AzFramework::WindowSize WindowContext::ResolveSwapchainSize()
        {
            AzFramework::WindowSize windowSize;
            AzFramework::WindowSize renderSize;
            AzFramework::WindowRequestBus::EventResult(
                windowSize,
                m_windowHandle,
                &AzFramework::WindowRequestBus::Events::GetClientAreaSize);
            AzFramework::WindowRequestBus::EventResult(
                renderSize,
                m_windowHandle,
                &AzFramework::WindowRequestBus::Events::GetRenderResolution);

            if (windowSize != renderSize && m_swapChainScalingMode == RHI::Scaling::None)
            {
                // no stretch support. we need to use the window size for render size
                renderSize = windowSize;
            }

            renderSize.m_width = AZStd::max(renderSize.m_width, 1u);
            renderSize.m_height = AZStd::max(renderSize.m_height, 1u);
            return renderSize;
        }

        void WindowContext::CreateSwapChains(RHI::Device& device)
        {
            RHI::Ptr<RHI::SwapChain> swapChain = aznew RHI::SwapChain;

            RHI::SwapChainDescriptor descriptor;

            AzFramework::WindowSize renderSize = ResolveSwapchainSize();
           
            const RHI::WindowHandle windowHandle = RHI::WindowHandle(reinterpret_cast<uintptr_t>(m_windowHandle));

            uint32_t syncInterval = 1;
            AzFramework::WindowRequestBus::EventResult(
                syncInterval, m_windowHandle, &AzFramework::WindowRequestBus::Events::GetSyncInterval);

            descriptor.m_window = windowHandle;
            descriptor.m_verticalSyncInterval = syncInterval;
            descriptor.m_dimensions.m_imageWidth = renderSize.m_width;
            descriptor.m_dimensions.m_imageHeight = renderSize.m_height;
            descriptor.m_dimensions.m_imageCount = AZStd::max(RHI::Limits::Device::MinSwapChainImages, RHI::Limits::Device::FrameCountMax);
            descriptor.m_dimensions.m_imageFormat = GetSwapChainFormat(device);
            descriptor.m_scalingMode = m_swapChainScalingMode;

            AZStd::string attachmentName = AZStd::string::format("WindowContextAttachment_%p", m_windowHandle);
            descriptor.m_attachmentId = RHI::AttachmentId{ attachmentName.c_str() };
            swapChain->Init(device.GetDeviceIndex(), descriptor);
            descriptor = swapChain->GetDescriptor(); // Get descriptor from swapchain because it can set different values during initialization

            RHI::Viewport viewport;
            viewport.m_maxX = static_cast<float>(descriptor.m_dimensions.m_imageWidth);
            viewport.m_maxY = static_cast<float>(descriptor.m_dimensions.m_imageHeight);

            RHI::Scissor scissor;
            scissor.m_maxX = static_cast<int16_t>(descriptor.m_dimensions.m_imageWidth);
            scissor.m_maxY = static_cast<int16_t>(descriptor.m_dimensions.m_imageHeight);

            uint32_t defaultSwapChainIndex = static_cast<uint32_t>(DefaultViewType);
            if (defaultSwapChainIndex < m_swapChainsData.size())
            {
                m_swapChainsData[defaultSwapChainIndex].m_swapChain = swapChain;
                m_swapChainsData[defaultSwapChainIndex].m_viewport = viewport;
                m_swapChainsData[defaultSwapChainIndex].m_scissor = scissor;
            }
            else
            {
                m_swapChainsData.insert(
                    m_swapChainsData.begin() + defaultSwapChainIndex, SwapChainData{ swapChain, viewport, scissor });
            }

            // Add XR pipelines if it is active
            XRRenderingInterface* xrSystem = RPISystemInterface::Get()->GetXRSystem();
            if (xrSystem)
            {
                const AZ::u32 numXrViews = xrSystem->GetNumViews();
                AZ_Assert(numXrViews <= 2, "Atom only supports two XR views");
                for (AZ::u32 i = 0; i < numXrViews; i++)
                {
                    RHI::Ptr<RHI::SwapChain> xrSwapChain = aznew RHI::SwapChain;
                    RHI::SwapChainDescriptor xrDescriptor;
                    xrDescriptor.m_dimensions.m_imageWidth = xrSystem->GetSwapChainWidth(i);
                    xrDescriptor.m_dimensions.m_imageHeight = xrSystem->GetSwapChainHeight(i);
                    xrDescriptor.m_dimensions.m_imageCount = AZ::RHI::Limits::Device::FrameCountMax;
                    xrDescriptor.m_isXrSwapChain = true;
                    xrDescriptor.m_xrSwapChainIndex = i;
                    xrDescriptor.m_dimensions.m_imageFormat = xrSystem->GetSwapChainFormat(i);
                    xrDescriptor.m_scalingMode = m_swapChainScalingMode;

                    const AZStd::string xrAttachmentName = AZStd::string::format("XRSwapChain_View_%i", i);
                    xrDescriptor.m_attachmentId = RHI::AttachmentId{ xrAttachmentName.c_str() };
                    xrSwapChain->Init(device.GetDeviceIndex(), xrDescriptor);
                    xrDescriptor = xrSwapChain->GetDescriptor(); // Get descriptor from swapchain because it can set different values during initialization

                    RHI::Viewport xrViewport;
                    xrViewport.m_maxX = static_cast<float>(xrDescriptor.m_dimensions.m_imageWidth);
                    xrViewport.m_maxY = static_cast<float>(xrDescriptor.m_dimensions.m_imageHeight);

                    RHI::Scissor xrScissor;
                    xrScissor.m_maxX = static_cast<int16_t>(xrDescriptor.m_dimensions.m_imageWidth);
                    xrScissor.m_maxY = static_cast<int16_t>(xrDescriptor.m_dimensions.m_imageHeight);

                    uint32_t xrSwapChainIndex = i == 0 ? static_cast<uint32_t>(ViewType::XrLeft) : static_cast<uint32_t>(ViewType::XrRight);
                    if (xrSwapChainIndex < m_swapChainsData.size())
                    {
                        m_swapChainsData[xrSwapChainIndex].m_swapChain = xrSwapChain;
                        m_swapChainsData[xrSwapChainIndex].m_viewport = xrViewport;
                        m_swapChainsData[xrSwapChainIndex].m_scissor = xrScissor;
                    }
                    else
                    {
                        m_swapChainsData.insert(m_swapChainsData.begin() + xrSwapChainIndex, SwapChainData{xrSwapChain, xrViewport, xrScissor});
                    }
                }
            }
        }

        void WindowContext::DestroyDefaultSwapChain()
        {
            DestroySwapChain(DefaultViewType);
        }

        void WindowContext::DestroyXRSwapChains()
        {
            DestroySwapChain(static_cast<uint32_t>(ViewType::XrLeft));
            DestroySwapChain(static_cast<uint32_t>(ViewType::XrRight));
        }

        void WindowContext::DestroySwapChain(uint32_t swapChainIndex)
        {
            if (swapChainIndex < m_swapChainsData.size())
            {
                m_swapChainsData[swapChainIndex].m_swapChain = nullptr;
            }   
        }

        void WindowContext::FillWindowState(const uint32_t width, const uint32_t height)
        {
            auto& defaultViewport = m_swapChainsData[DefaultViewType].m_viewport;
            auto& defaultScissor = m_swapChainsData[DefaultViewType].m_scissor;

            defaultViewport.m_minX = 0;
            defaultViewport.m_minY = 0;
            defaultViewport.m_maxX = static_cast<float>(width);
            defaultViewport.m_maxY = static_cast<float>(height);

            defaultScissor.m_minX = 0;
            defaultScissor.m_minY = 0;
            defaultScissor.m_maxX = static_cast<int16_t>(width);
            defaultScissor.m_maxY = static_cast<int16_t>(height);
        }

        RHI::Format WindowContext::GetSwapChainFormat(RHI::Device& device) const
        {
            // Array of preferred format in decreasing order of preference.
            const AZStd::array<RHI::Format, 3> preferredFormats =
            {{
                RHI::Format::R10G10B10A2_UNORM,
                RHI::Format::R8G8B8A8_UNORM,
                RHI::Format::B8G8R8A8_UNORM
            }};

            auto GetPreferredFormat = [](const AZStd::array<RHI::Format, 3>& preferredFormats, const AZStd::vector<RHI::Format>& supportedFormats) -> RHI::Format
            {
                for (int preferredIndex = 0; preferredIndex < preferredFormats.size(); ++preferredIndex)
                {
                    for (int supportedIndex = 0; supportedIndex < supportedFormats.size(); ++supportedIndex)
                    {
                        if (supportedFormats[supportedIndex] == preferredFormats[preferredIndex])
                        {
                            return supportedFormats[supportedIndex];
                        }
                    }
                }
                // If no match found, just return the first supported format
                return supportedFormats[0];
            };

            const RHI::WindowHandle windowHandle = RHI::WindowHandle(reinterpret_cast<uintptr_t>(m_windowHandle));
            const AZStd::vector<RHI::Format> supportedFormats = device.GetValidSwapChainImageFormats(windowHandle);
            AZ_Assert(!supportedFormats.empty(), "There is no supported format for SwapChain images.");

            return GetPreferredFormat(preferredFormats, supportedFormats);
        }
    } // namespace RPI
} // namespace AZ
