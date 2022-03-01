/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/FrameScheduler.h>

#include <Atom/RPI.Public/WindowContext.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Pass/AttachmentReadback.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/Specific/SwapChainPass.h>
#include <Atom/RHI/RHISystemInterface.h>

namespace AZ
{
    namespace RPI
    {
        SwapChainPass::SwapChainPass(const PassDescriptor& descriptor, const WindowContext* windowContext)
            : ParentPass(descriptor)
            , m_windowContext(windowContext)
        {
            AzFramework::WindowNotificationBus::Handler::BusConnect(m_windowContext->GetWindowHandle());
        }

        Ptr<ParentPass> SwapChainPass::Recreate() const
        {
            PassDescriptor desc = GetPassDescriptor();
            Ptr<ParentPass> pass = aznew SwapChainPass(desc, m_windowContext);
            return pass;
        }

        SwapChainPass::~SwapChainPass()
        {
            AzFramework::WindowNotificationBus::Handler::BusDisconnect();
        }

        RHI::Format SwapChainPass::GetSwapChainFormat() const
        {
            if (m_attachmentBindings.size() > 0 && m_attachmentBindings[0].GetAttachment())
            {
                return m_attachmentBindings[0].GetAttachment()->GetTransientImageDescriptor().m_imageDescriptor.m_format;
            }
            return RHI::Format::Unknown;
        }

        const RHI::Scissor& SwapChainPass::GetScissor() const
        {
            return m_scissorState;
        }

        const RHI::Viewport& SwapChainPass::GetViewport() const
        {
            return m_viewportState;
        }

        void SwapChainPass::SetupSwapChainAttachment()
        {
            m_swapChainDimensions = m_windowContext->GetSwapChain()->GetDescriptor().m_dimensions;

            m_swapChainAttachment = aznew PassAttachment();
            m_swapChainAttachment->m_name = "SwapChainOutput";
            m_swapChainAttachment->m_path = m_windowContext->GetSwapChainAttachmentId();

            RHI::ImageDescriptor swapChainImageDesc;
            swapChainImageDesc.m_bindFlags = RHI::ImageBindFlags::Color | RHI::ImageBindFlags::ShaderRead | RHI::ImageBindFlags::CopyWrite;
            swapChainImageDesc.m_size.m_width = m_swapChainDimensions.m_imageWidth;
            swapChainImageDesc.m_size.m_height = m_swapChainDimensions.m_imageHeight;
            swapChainImageDesc.m_format = m_swapChainDimensions.m_imageFormat;
            m_swapChainAttachment->m_descriptor = swapChainImageDesc;

            PassAttachmentBinding* swapChainOutput = FindAttachmentBinding(Name("SwapChainOutput"));
            AZ_Assert(swapChainOutput != nullptr &&
                      swapChainOutput->m_slotType == PassSlotType::InputOutput,
                      "PassTemplate used to create SwapChainPass must have an InputOutput called SwapChainOutput");

            swapChainOutput->SetAttachment(m_swapChainAttachment);
        }

        // --- Pass behavior overrides ---

        void SwapChainPass::BuildInternal()
        {
            if (m_windowContext->GetSwapChain() == nullptr)
            {
                return;
            }

            m_scissorState = m_windowContext->GetScissor();
            m_viewportState = m_windowContext->GetViewport();

            SetupSwapChainAttachment();

            ParentPass::BuildInternal();
        }

        void SwapChainPass::FrameBeginInternal(FramePrepareParams params)
        {
            params.m_scissorState = m_scissorState;
            params.m_viewportState = m_viewportState;

            if(m_windowContext->GetSwapChain() == nullptr || m_windowContext->GetSwapChain()->GetImageCount() == 0)
            {
                return;
            }

            RHI::FrameGraphAttachmentInterface attachmentDatabase = params.m_frameGraphBuilder->GetAttachmentDatabase();

            // Import the SwapChain
            attachmentDatabase.ImportSwapChain(m_windowContext->GetSwapChainAttachmentId(), m_windowContext->GetSwapChain());

            ParentPass::FrameBeginInternal(params);
        }
        
        void SwapChainPass::OnWindowResized([[maybe_unused]] uint32_t width, [[maybe_unused]] uint32_t height)
        {
            QueueForBuildAndInitialization();
        }

        void SwapChainPass::ReadbackSwapChain(AZStd::shared_ptr<AttachmentReadback> readback)
        {
            if (m_swapChainAttachment)
            {
                m_readbackOption = PassAttachmentReadbackOption::Output;
                m_attachmentReadback = readback;

                AZStd::string readbackName = AZStd::string::format("%s_%s", m_swapChainAttachment->GetAttachmentId().GetCStr(), GetName().GetCStr());
                m_attachmentReadback->ReadPassAttachment(m_swapChainAttachment.get(), AZ::Name(readbackName));
            }
        }

        AzFramework::NativeWindowHandle SwapChainPass::GetWindowHandle() const
        {
            return m_windowContext->GetWindowHandle();
        }

    }   // namespace RPI
}   // namespace AZ
