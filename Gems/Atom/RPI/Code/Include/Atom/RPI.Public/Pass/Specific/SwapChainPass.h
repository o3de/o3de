/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/ScopeProducerFunction.h>
#include <Atom/RHI/ShaderResourceGroup.h>

#include <Atom/RHI.Reflect/SwapChainDescriptor.h>

#include <Atom/RPI.Public/Pass/AttachmentReadback.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>

#include <AzFramework/Windowing/WindowBus.h>

namespace AZ
{
    namespace RHI
    {
        class SwapChain;
        struct ImageScopeAttachmentDescriptor;
    }

    namespace RPI
    {
        class WindowContext;

        //! Pass that outputs to a SwapChain
        //! Holds all the passes needed to render a frame like depth, forward, post effects etc.
        class SwapChainPass final
            : public ParentPass
            , public AzFramework::WindowNotificationBus::Handler
        {
        public:
            AZ_RTTI(SwapChainPass, "{551AD61F-8603-4998-A7D1-226F03022295}", ParentPass);
            AZ_CLASS_ALLOCATOR(SwapChainPass, SystemAllocator, 0);

            SwapChainPass(const PassDescriptor& descriptor, const WindowContext* windowContext, const Name& childTemplateName);
            ~SwapChainPass();

            Ptr<ParentPass> Recreate() const override;

            const RHI::Scissor& GetScissor() const;
            const RHI::Viewport& GetViewport() const;

            void ReadbackSwapChain(AZStd::shared_ptr<AttachmentReadback> readback);

            AzFramework::NativeWindowHandle GetWindowHandle() const;

        protected:
            // Pass behavior overrides
            void CreateChildPassesInternal() override final;
            void BuildAttachmentsInternal() override final;
            void FrameBeginInternal(FramePrepareParams params) override final;
            
            // WindowNotificationBus::Handler overrides ...
            void OnWindowResized(uint32_t width, uint32_t height) override;

        private:

            // Sets up a swap chain PassAttachment using the swap chain id from the window context 
            void SetupSwapChainAttachment();

            // The WindowContext that owns the SwapChain this pass renders to
            const WindowContext* m_windowContext = nullptr;

            // The SwapChain used when rendering this pass
            Ptr<PassAttachment> m_swapChainAttachment;

            RHI::SwapChainDimensions m_swapChainDimensions;

            RHI::Scissor m_scissorState;
            RHI::Viewport m_viewportState;

            bool m_postProcess = false;

            // The master child pass used to drive rendering for this swapchain
            Ptr<Pass> m_childPass = nullptr;

            // Name of the template used to create the child pass. Needed for Recreate()
            Name m_childTemplateName;

            // For read back swap chain
            AZStd::shared_ptr<AttachmentReadback> m_swapChainReadback;
        };

    }   // namespace RPI
}   // namespace AZ
