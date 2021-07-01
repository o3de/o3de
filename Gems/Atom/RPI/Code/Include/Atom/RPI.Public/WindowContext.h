/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/Base.h>

#include <Atom/RHI/SwapChain.h>

#include <Atom/RHI.Reflect/Scissor.h>
#include <Atom/RHI.Reflect/Viewport.h>
#include <Atom/RHI.Reflect/AttachmentId.h>

#include <AzFramework/Windowing/WindowBus.h>

namespace AZ
{
    namespace RPI
    {
        //! An RPI-level understanding of a Window and its relevant SwapChain
        //! This is how the RPI creates a SwapChain from an AzFramework's Window
        //! and passes it down throughout the RPI. This is a convenient way to
        //! package the SwapChain, Window size, Attachment info and other
        //! window information needed to present to a surface.
        class WindowContext final
            : public AzFramework::WindowNotificationBus::Handler
            , public AzFramework::ExclusiveFullScreenRequestBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(WindowContext, AZ::SystemAllocator, 0);

            WindowContext() = default;
            ~WindowContext() = default;

            //! Initializes the WindowContext from the given AzFramework's window
            //! param[in] windowHandle The native window handle of the Window we want to construct an RPI WindowContext for
            //! param[in] masterPassName The name of the pass that supplies input to the window's swapchain pass
            void Initialize(RHI::Device& device, AzFramework::NativeWindowHandle windowHandle);

            //! Gets the ViewportContexts associated with this WindowContext, if any.
            //! Associated ViewportContexts can be enumerated to find the active rendered scene.
            AZStd::vector<ViewportContextPtr> GetAssociatedViewportContexts();

            //! Registers a ViewportContext as associated with this WindowContext.
            void RegisterAssociatedViewportContext(ViewportContextPtr viewportContext);

            //! Shuts down the WindowContext and destroys the underlying SwapChain
            void Shutdown();

            //! Returns a unique attachment id associated with the swap chain.
            const RHI::AttachmentId& GetSwapChainAttachmentId() const;

            //! Retrieves the underlying SwapChain created by this WindowContext
            const RHI::Ptr<RHI::SwapChain>& GetSwapChain() const;

            //! Retrieves the default ViewportState for the WindowContext
            const RHI::Viewport& GetViewport() const;

            //! Retrieves the default ScissorState for the WindowContext
            const RHI::Scissor& GetScissor() const;

            //! Get the window ID for the WindowContext
            AzFramework::NativeWindowHandle GetWindowHandle() const { return m_windowHandle; }

        private:

            // WindowNotificationBus::Handler overrides ...
            void OnWindowResized(uint32_t width, uint32_t height) override;
            void OnWindowClosed() override;

            // ExclusiveFullScreenRequestBus::Handler overrides ...
            bool IsExclusiveFullScreenPreferred() const override;
            bool GetExclusiveFullScreenState() const override;
            bool SetExclusiveFullScreenState(bool fullScreenState) override;

            // Creates the underlying RHI level SwapChain for the given Window
            void CreateSwapChain(RHI::Device& device);

            // Destroys the underlying SwapChain
            void DestroySwapChain();

            // Fills the default window states based on the given width and height
            void FillWindowState(const uint32_t width, const uint32_t height);

            // Get the swapchain format to use
            RHI::Format GetSwapChainFormat(RHI::Device& device) const;

            // The handle of the window that this is context describes
            AzFramework::NativeWindowHandle m_windowHandle;

            // The swapChain that this context has created for the given window
            RHI::Ptr<RHI::SwapChain> m_swapChain;

            // The default viewport that covers the entire surface described by the SwapChain
            RHI::Viewport m_viewportDefault;

            // The default scissor that covers the entire surface described by the SwapChain
            RHI::Scissor m_scissorDefault;

            // Non-owning reference to associated ViewportContexts (if any)
            AZStd::vector<AZStd::weak_ptr<ViewportContext>> m_viewportContexts;
        };

        using WindowContextSharedPtr = AZStd::shared_ptr<WindowContext>;

    } // namespace RPI
} // namespace AZ
