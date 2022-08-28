/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/FrameScheduler.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Pass/AttachmentReadback.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/Specific/SwapChainPass.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/RPISystemInterface.h>

#include <AzCore/Console/Console.h>

AZ_CVAR(
    float,
    r_renderScale,
    2.f,
    nullptr,
    AZ::ConsoleFunctorFlags::DontReplicate,
    "When set to a number greater than 1.0 (the minimum allowed), renders the scene at a reduced resolution, "
    "relying on FSR2 or some other pass to upscale the rendered output to the final display.");

AZ_CVAR(
    float,
    r_renderScaleMin,
    1.f,
    nullptr,
    AZ::ConsoleFunctorFlags::DontReplicate,
    "The minimum render scale can be used to enforce the maximum resolution the scene can render at prior to upscale. Increasing "
    "this minimum from 1.0 can be used to conserve some memory needed to allocate transient render targets.");

AZ_CVAR(
    float,
    r_renderScaleMax,
    3.f,
    nullptr,
    AZ::ConsoleFunctorFlags::DontReplicate,
    "The render scale allowed is capped at this amount (defaults to a maximum 3x upscale).");

namespace AZ
{
    namespace RPI
    {
        SwapChainPass::SwapChainPass(
            const PassDescriptor& descriptor, const WindowContext* windowContext, const WindowContext::SwapChainMode swapChainMode)
            : ParentPass(descriptor)
            , m_windowContext(windowContext)
            , m_swapChainMode(swapChainMode)
        {
            AzFramework::WindowNotificationBus::Handler::BusConnect(m_windowContext->GetWindowHandle());
        }

        Ptr<ParentPass> SwapChainPass::Recreate() const
        {
            PassDescriptor desc = GetPassDescriptor();
            Ptr<ParentPass> pass = aznew SwapChainPass(desc, m_windowContext, m_swapChainMode);
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
            m_swapChainDimensions = m_windowContext->GetSwapChain(m_swapChainMode)->GetDescriptor().m_dimensions;

            m_swapChainAttachment = aznew PassAttachment();
            m_swapChainAttachment->m_name = "SwapChainOutput";
            m_swapChainAttachment->m_path = m_windowContext->GetSwapChainAttachmentId(m_swapChainMode);

            RHI::ImageDescriptor swapChainImageDesc;
            swapChainImageDesc.m_bindFlags = RHI::ImageBindFlags::Color | RHI::ImageBindFlags::ShaderRead | RHI::ImageBindFlags::CopyWrite;
            swapChainImageDesc.m_size.m_width = m_swapChainDimensions.m_imageWidth;
            swapChainImageDesc.m_size.m_height = m_swapChainDimensions.m_imageHeight;
            swapChainImageDesc.m_format = m_swapChainDimensions.m_imageFormat;
            m_swapChainAttachment->m_descriptor = swapChainImageDesc;

            PassAttachmentBinding* swapChainOutput = FindAttachmentBinding(Name("PipelineOutput"));
            AZ_Assert(swapChainOutput != nullptr &&
                      swapChainOutput->m_slotType == PassSlotType::InputOutput,
                      "PassTemplate used to create SwapChainPass must have an InputOutput called PipelineOutput");

            PassAttachmentBinding* renderOutput = FindAttachmentBinding(Name("RenderOutput"));
            if (renderOutput)
            {
                // A pipeline may optionally define a render output attachment as a logical input/output.
                // This attachment isn't actually tied to a physically backed render target, but is used
                // to control render resolution, which may be lower than the final pipeline output. Passes
                // that render the scene prior to UI and various post-processing passes will target the
                // render output attachment by default.

                if (!m_renderOutputAttachment)
                {
                    m_renderOutputAttachment = aznew PassAttachment;
                    m_renderOutputAttachment->m_name = "RenderOutput";
                    m_renderOutputAttachment->m_path = "RenderOutput";
                }

                // Initialize the render output descriptor to the swapchain image descriptor. The actual
                // render scale is adjusted each frame.
                m_renderOutputAttachment->m_descriptor.m_image = swapChainImageDesc;

                renderOutput->SetAttachment(m_renderOutputAttachment);
            }

            swapChainOutput->SetAttachment(m_swapChainAttachment);
        }

        // --- Pass behavior overrides ---

        void SwapChainPass::BuildInternal()
        {
            if (m_windowContext->GetSwapChainsSize() == 0 ||
                m_windowContext->GetSwapChain(m_swapChainMode) == nullptr)
            {
                return;
            }

            m_scissorState = m_windowContext->GetScissor(m_swapChainMode);
            m_viewportState = m_windowContext->GetViewport(m_swapChainMode);

            SetupSwapChainAttachment();

            if (m_renderOutputAttachment)
            {

                // If the pipeline exposes a RenderOutput attachment, modulate the render target
                // size based on the r_renderScale cvars.
                m_currentRenderScale = AZ::GetClamp((float)r_renderScale, (float)r_renderScaleMin, (float)r_renderScaleMax);
                float invRenderScale = 1.f / m_currentRenderScale;

                m_renderOutputAttachment->m_descriptor.m_image.m_size.m_width =
                    aznumeric_cast<uint32_t>(invRenderScale * m_swapChainDimensions.m_imageWidth);
                m_renderOutputAttachment->m_descriptor.m_image.m_size.m_height =
                    aznumeric_cast<uint32_t>(invRenderScale * m_swapChainDimensions.m_imageHeight);
            }

            ParentPass::BuildInternal();
        }

        void SwapChainPass::FrameBeginInternal(FramePrepareParams params)
        {
            params.m_scissorState = m_scissorState;
            params.m_viewportState = m_viewportState;

            if (m_windowContext->GetSwapChainsSize() == 0 ||
                m_windowContext->GetSwapChain(m_swapChainMode) == nullptr ||
                m_windowContext->GetSwapChain(m_swapChainMode)->GetImageCount() == 0)
            {
                return;
            }

            if (m_renderOutputAttachment && r_renderScale != m_currentRenderScale)
            {
                QueueForBuildAndInitialization();
            }

            RHI::FrameGraphAttachmentInterface attachmentDatabase = params.m_frameGraphBuilder->GetAttachmentDatabase();

            AZ::RPI::RPISystemInterface* rpiSystem = AZ::RPI::RPISystemInterface::Get();
            if (rpiSystem->GetXRSystem())
            {
                switch (m_swapChainMode)
                {
                case WindowContext::SwapChainMode::XrLeft:
                    {
                        rpiSystem->GetXRSystem()->AcquireSwapChainImage(0);
                        break;
                    }
                case WindowContext::SwapChainMode::XrRight:
                    {
                        rpiSystem->GetXRSystem()->AcquireSwapChainImage(1);
                        break;
                    }
                default:
                    {
                        // No need to do anything for non-xr swapchain
                        break;
                    }
                } 
            }

            // Import the SwapChain
            attachmentDatabase.ImportSwapChain(
                m_windowContext->GetSwapChainAttachmentId(m_swapChainMode), m_windowContext->GetSwapChain(m_swapChainMode));

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
