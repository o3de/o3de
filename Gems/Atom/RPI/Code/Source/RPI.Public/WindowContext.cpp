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

#include <AzCore/Console/IConsole.h>
#include <AzCore/Math/MathUtils.h>

namespace AZ
{
    namespace RPI
    {
        void WindowContext::Initialize(RHI::Device& device, AzFramework::NativeWindowHandle windowHandle)
        {
            m_windowHandle = windowHandle;

            CreateSwapChain(device);

            FillWindowState(m_swapChain->GetDescriptor().m_dimensions.m_imageWidth, m_swapChain->GetDescriptor().m_dimensions.m_imageHeight);

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

            DestroySwapChain();
        }

        const RHI::AttachmentId& WindowContext::GetSwapChainAttachmentId() const
        {
            return m_swapChain->GetAttachmentId();
        }

        const RHI::Ptr<RHI::SwapChain>& WindowContext::GetSwapChain() const
        {
            return m_swapChain;
        }

        const RHI::Viewport& WindowContext::GetViewport() const
        {
            return m_viewportDefault;
        }

        const RHI::Scissor& WindowContext::GetScissor() const
        {
            return m_scissorDefault;
        }

        void WindowContext::OnWindowResized(uint32_t width, uint32_t height)
        {
            const AZ::RHI::SwapChainDimensions& currentDimensions = m_swapChain->GetDescriptor().m_dimensions;
            if (width != currentDimensions.m_imageWidth || height != currentDimensions.m_imageHeight)
            {
                // Get current dimension and only overwrite the sizes.
                RHI::SwapChainDimensions dimensions = m_swapChain->GetDescriptor().m_dimensions;
                dimensions.m_imageWidth = AZStd::max(width, 1u);
                dimensions.m_imageHeight = AZStd::max(height, 1u);
                dimensions.m_imageFormat = GetSwapChainFormat(m_swapChain->GetDevice());

                FillWindowState(dimensions.m_imageWidth, dimensions.m_imageHeight);

                m_swapChain->Resize(dimensions);

                WindowContextNotificationBus::Event(m_windowHandle, &WindowContextNotifications::OnViewportResized, width, height);
            }
        }

        void WindowContext::OnWindowClosed()
        {
            DestroySwapChain();

            // We don't want to listen to events anymore if the window has closed
            AzFramework::ExclusiveFullScreenRequestBus::Handler::BusDisconnect(m_windowHandle);
            AzFramework::WindowNotificationBus::Handler::BusDisconnect(m_windowHandle);
        }

        void WindowContext::OnVsyncIntervalChanged(uint32_t interval)
        {
            if (m_swapChain->GetDescriptor().m_verticalSyncInterval != interval)
            {
                m_swapChain->SetVerticalSyncInterval(interval);
            }
        }

        bool WindowContext::IsExclusiveFullScreenPreferred() const
        {
            return m_swapChain->IsExclusiveFullScreenPreferred();
        }

        bool WindowContext::GetExclusiveFullScreenState() const
        {
            return m_swapChain->GetExclusiveFullScreenState();
        }

        bool WindowContext::SetExclusiveFullScreenState(bool fullScreenState)
        {
            return m_swapChain->SetExclusiveFullScreenState(fullScreenState);
        }

        void WindowContext::CreateSwapChain(RHI::Device& device)
        {
            m_swapChain = RHI::Factory::Get().CreateSwapChain();

            AzFramework::WindowSize windowSize;
            AzFramework::WindowRequestBus::EventResult(
                windowSize,
                m_windowHandle,
                &AzFramework::WindowRequestBus::Events::GetClientAreaSize);

            uint32_t width = AZStd::max(windowSize.m_width, 1u);
            uint32_t height = AZStd::max(windowSize.m_height, 1u);

            const RHI::WindowHandle windowHandle = RHI::WindowHandle(reinterpret_cast<uintptr_t>(m_windowHandle));

            uint32_t syncInterval = 1;
            AzFramework::WindowRequestBus::EventResult(
                syncInterval, m_windowHandle, &AzFramework::WindowRequestBus::Events::GetSyncInterval);

            RHI::SwapChainDescriptor descriptor;
            descriptor.m_window = windowHandle;
            descriptor.m_verticalSyncInterval = syncInterval;
            descriptor.m_dimensions.m_imageWidth = width;
            descriptor.m_dimensions.m_imageHeight = height;
            descriptor.m_dimensions.m_imageCount = 3;
            descriptor.m_dimensions.m_imageFormat = GetSwapChainFormat(device);

            AZStd::string attachmentName = AZStd::string::format("WindowContextAttachment_%p", m_windowHandle);
            descriptor.m_attachmentId = RHI::AttachmentId{ attachmentName.c_str() };

            m_swapChain->Init(device, descriptor);
        }

        void WindowContext::DestroySwapChain()
        {
            m_swapChain = nullptr;
        }

        void WindowContext::FillWindowState(const uint32_t width, const uint32_t height)
        {
            m_viewportDefault.m_minX = 0;
            m_viewportDefault.m_minY = 0;
            m_viewportDefault.m_maxX = static_cast<float>(width);
            m_viewportDefault.m_maxY = static_cast<float>(height);

            m_scissorDefault.m_minX = 0;
            m_scissorDefault.m_minY = 0;
            m_scissorDefault.m_maxX = static_cast<int16_t>(width);
            m_scissorDefault.m_maxY = static_cast<int16_t>(height);
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
