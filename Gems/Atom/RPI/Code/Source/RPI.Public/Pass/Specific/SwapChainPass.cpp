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
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/Specific/SwapChainPass.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/RPISystemInterface.h>

namespace AZ
{
    namespace RPI
    {
        SwapChainPass::SwapChainPass(const PassDescriptor& descriptor, const WindowContext* windowContext, const ViewType viewType)
            : ParentPass(descriptor)
            , m_windowContext(windowContext)
            , m_viewType(viewType)
        {
            AzFramework::WindowNotificationBus::Handler::BusConnect(m_windowContext->GetWindowHandle());
        }

        Ptr<ParentPass> SwapChainPass::Recreate() const
        {
            PassDescriptor desc = GetPassDescriptor();
            Ptr<ParentPass> pass = aznew SwapChainPass(desc, m_windowContext, m_viewType);
            return pass;
        }

        SwapChainPass::~SwapChainPass()
        {
            AzFramework::WindowNotificationBus::Handler::BusDisconnect();
        }

        RHI::Format SwapChainPass::GetSwapChainFormat() const
        {
            if (m_swapChainAttachment)
            {
                return m_swapChainAttachment->m_descriptor.m_image.m_format;
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
            // Find "PipelineOutput" slot from the render pipeline's root pass 
            PassAttachmentBinding* pipelineOutput = FindAttachmentBinding(Name("PipelineOutput"));
            AZ_Assert(pipelineOutput != nullptr &&
                      pipelineOutput->m_slotType == PassSlotType::InputOutput,
                      "PassTemplate used to create SwapChainPass must have an InputOutput called PipelineOutput");

            AzFramework::WindowSize renderSize;
            AzFramework::WindowRequestBus::EventResult(
                renderSize, m_windowContext->GetWindowHandle(), &AzFramework::WindowRequestBus::Events::GetRenderResolution);

            RHI::SwapChainDimensions swapChainDimensions = m_windowContext->GetSwapChain(m_viewType)->GetDescriptor().m_dimensions;

            // If the swapchain doesn't support scaling and render resolution is different than swapchain size
            m_needResize = m_windowContext->GetSwapChainScalingMode() == RHI::Scaling::None &&
                (swapChainDimensions.m_imageWidth != renderSize.m_width || swapChainDimensions.m_imageHeight != renderSize.m_height);

            // Note: we can't add m_swapChainAttachment to m_ownedAttachments then it would be imported to frame graph's attachment database as a regular image. 
            m_swapChainAttachment = aznew PassAttachment();
            m_swapChainAttachment->m_name = "SwapChainOutput";
            m_swapChainAttachment->m_path = m_windowContext->GetSwapChainAttachmentId(m_viewType);

            RHI::ImageDescriptor swapChainImageDesc;
            swapChainImageDesc.m_bindFlags = RHI::ImageBindFlags::Color | RHI::ImageBindFlags::ShaderRead | RHI::ImageBindFlags::CopyWrite;
            swapChainImageDesc.m_size.m_width = swapChainDimensions.m_imageWidth;
            swapChainImageDesc.m_size.m_height = swapChainDimensions.m_imageHeight;
            swapChainImageDesc.m_format = swapChainDimensions.m_imageFormat;
            m_swapChainAttachment->m_descriptor = swapChainImageDesc;

            if (m_needResize)
            {
                // Create a new binding for swapchain output
                // It's used to connect to child pass's Output slot
                PassAttachmentBinding outputBinding;
                outputBinding.m_name = "SwapChainOutput";
                outputBinding.m_slotType = PassSlotType::Output;
                outputBinding.m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::RenderTarget;
                outputBinding.SetAttachment(m_swapChainAttachment);
                AddAttachmentBinding(outputBinding);

                // create an intermediate attachment which has window render resolution
                RHI::ImageDescriptor outputImageDesc;
                outputImageDesc = swapChainImageDesc;
                outputImageDesc.m_size.m_width = renderSize.m_width;
                outputImageDesc.m_size.m_height = renderSize.m_height;
                m_pipelinOutputAttachment = aznew PassAttachment();
                m_pipelinOutputAttachment->m_lifetime = RHI::AttachmentLifetimeType::Transient;
                m_pipelinOutputAttachment->m_descriptor = outputImageDesc;
                m_pipelinOutputAttachment->m_name = "PipelineOutput";
                m_pipelinOutputAttachment->ComputePathName(GetPathName());
                m_ownedAttachments.push_back(m_pipelinOutputAttachment);

                // use the intermediate attachment as pipeline's output
                pipelineOutput->SetAttachment(m_pipelinOutputAttachment);
            }
            else
            {
                // use the intermediate attachment as pipeline's output
                pipelineOutput->SetAttachment(m_swapChainAttachment);
            }
        }

        void SwapChainPass::BuildInternal()
        {
            if (m_windowContext->GetSwapChainsSize() == 0 || m_windowContext->GetSwapChain(m_viewType) == nullptr)
            {
                return;
            }

            m_scissorState = m_windowContext->GetScissor(m_viewType);
            m_viewportState = m_windowContext->GetViewport(m_viewType);

            SetupSwapChainAttachment();

            ParentPass::BuildInternal();

            // use the SwapchainOutput as the output of CopyToSwapChain pass to support swapchain scaling
            if (m_needResize)
            {
                RPI::PassFilter passFilter = RPI::PassFilter::CreateWithPassName(Name("CopyToSwapChain"), GetRenderPipeline());
                RPI::Ptr<RPI::Pass> copyPass = RPI::PassSystemInterface::Get()->FindFirstPass(passFilter);
                if (copyPass)
                {
                    copyPass->SetEnabled(true);
                    auto outputBinding = copyPass->FindAttachmentBinding(Name("Output"));
                    outputBinding->SetAttachment(m_swapChainAttachment);
                    // set connected binding to nullptr so it won't use the output attachment specified in the pipeline's pass template
                    outputBinding->m_connectedBinding = nullptr;
                }
                else
                {
                    AZ_Error("SwapChainPass", false, "Render pipeline requires 'CopyToSwapChain' pass with 'Output' slot to support swapchain resize");
                }
            }

            if (m_pipeline)
            {
                m_pipeline->UpdateViewportScissor();
            }
        }

        void SwapChainPass::FrameBeginInternal(FramePrepareParams params)
        {
            if (m_windowContext->GetSwapChainsSize() == 0 || m_windowContext->GetSwapChain(m_viewType) == nullptr ||
                m_windowContext->GetSwapChain(m_viewType)->GetImageCount() == 0)
            {
                return;
            }

            RHI::FrameGraphAttachmentInterface attachmentDatabase = params.m_frameGraphBuilder->GetAttachmentDatabase();

            AZ::RPI::RPISystemInterface* rpiSystem = AZ::RPI::RPISystemInterface::Get();
            if (rpiSystem->GetXRSystem())
            {
                switch (m_viewType)
                {
                case ViewType::XrLeft:
                    {
                        rpiSystem->GetXRSystem()->AcquireSwapChainImage(0);
                        break;
                    }
                case ViewType::XrRight:
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
                m_windowContext->GetSwapChainAttachmentId(m_viewType), m_windowContext->GetSwapChain(m_viewType));

            ParentPass::FrameBeginInternal(params);
        }
        
        void SwapChainPass::OnResolutionChanged([[maybe_unused]] uint32_t width, [[maybe_unused]] uint32_t height)
        {
            QueueForBuildAndInitialization();
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
