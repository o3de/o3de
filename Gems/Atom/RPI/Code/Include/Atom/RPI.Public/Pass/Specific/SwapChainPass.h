/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/ScopeProducerFunction.h>

#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RHI.Reflect/SwapChainDescriptor.h>

#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/WindowContext.h>

#include <AzFramework/Windowing/WindowBus.h>

namespace AZ
{
    namespace RPI
    {
        //! SwapChainPass is the root pass for a render pipeline which outputs to a swapchain.
        //! It creates a swapchain attachment and uses it for the PipelineOutput binding of the RenderPipeline
        //! Restrictions:
        //! - The pass template should have a slot name "PipelineOutput"
        //! - To support explicit render resolution ( swapchain has different size than the render pipeline's resolution),
        //!     the pass template should have a pass request which creates the "CopyToSwapChain" pass
        class ATOM_RPI_PUBLIC_API SwapChainPass final
            : public ParentPass
            , public AzFramework::WindowNotificationBus::Handler
        {
        public:
            AZ_RTTI(SwapChainPass, "{551AD61F-8603-4998-A7D1-226F03022295}", ParentPass);
            AZ_CLASS_ALLOCATOR(SwapChainPass, SystemAllocator);

            SwapChainPass(const PassDescriptor& descriptor, const WindowContext* windowContext, const ViewType viewType);
            ~SwapChainPass();

            Ptr<ParentPass> Recreate() const override;

            const RHI::Scissor& GetScissor() const;
            const RHI::Viewport& GetViewport() const;

            void ReadbackSwapChain(AZStd::shared_ptr<AttachmentReadback> readback);

            AzFramework::NativeWindowHandle GetWindowHandle() const;

            RHI::Format GetSwapChainFormat() const;

        protected:
            // Pass behavior overrides
            void BuildInternal() override final;
            void FrameBeginInternal(FramePrepareParams params) override final;
            
            // WindowNotificationBus::Handler overrides ...
            // The m_pipelinOutputAttachment need to be recreated when render resolution changed
            void OnResolutionChanged(uint32_t width, uint32_t height) override;
            // Swapchain may get resized when window is resized
            void OnWindowResized(uint32_t width, uint32_t height) override;

        private:
            // Sets up a swap chain PassAttachment using the swap chain id from the window context 
            void SetupSwapChainAttachment();

            // The WindowContext that owns the SwapChain this pass renders to
            const WindowContext* m_windowContext = nullptr;

            // The SwapChain used when rendering this pass
            Ptr<PassAttachment> m_swapChainAttachment;

            // The intermediate attachment used for render pipeline's output
            Ptr<PassAttachment> m_pipelinOutputAttachment;

            // The swapchain pass need to do resize from pipeline output to device swapchain
            bool m_needResize = false;

            RHI::Scissor m_scissorState;
            RHI::Viewport m_viewportState;
            ViewType m_viewType = ViewType::Default;
        };

    }   // namespace RPI
}   // namespace AZ
