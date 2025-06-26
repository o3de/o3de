/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/RHIUtils.h>
#include <Atom/RPI.Public/Pass/CopyPass.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/Specific/SwapChainPass.h>
#include <Atom/RPI.Public/PipelinePassChanges.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Pass/CopyPassData.h>

#include <AFR/AFRFeatureProcessor.h>

namespace AFR
{
    void AFRFeatureProcessor::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AFRFeatureProcessor, AZ::RPI::FeatureProcessor>()->Version(0);
        }
    }

    void AFRFeatureProcessor::Activate()
    {
        // Check if AFR should be active
        m_afrPipelineName = AZ::RHI::GetCommandLineValue("afr");
        if (m_afrPipelineName.empty())
        {
            if (!AZ::RHI::QueryCommandLineOption("afr"))
                return;
        }

        m_deviceCount = AZ::RHI::RHISystemInterface::Get()->GetDeviceCount();

        // Enable scene notifications to get notified on RenderPipeline changes and add CopyPasses as needed
        EnableSceneNotification();

        // Connect to the FrameEventBus to access the OnFrameBegin() event to affect the scheduling of passes
        AZ::RHI::FrameEventBus::Handler::BusConnect(
            AZ::RHI::RHISystemInterface::Get()->GetDevice(AZ::RHI::MultiDevice::DefaultDeviceIndex));
    }

    void AFRFeatureProcessor::Deactivate()
    {
        DisableSceneNotification();

        AZ::RHI::FrameEventBus::Handler::BusDisconnect();
    }

    void AFRFeatureProcessor::OnRenderPipelineChanged(AZ::RPI::RenderPipeline* renderPipeline, RenderPipelineChangeType pipelineChangeType)
    {
        switch (pipelineChangeType)
        {
        case RenderPipelineChangeType::Added:
            {
                if (renderPipeline->GetId().GetStringView().contains(m_afrPipelineName))
                {
                    if(m_afrRenderPipeline.second)
                    {
                        AZ_Assert(!m_afrPipelineName.empty(), "AFR Pipeline had already been set, reset to this pipeline");
                        m_afrRenderPipeline = { "", nullptr };
                        m_afrCopyPasses.clear();
                    }

                    AZ::RPI::PassFilter passFilter = AZ::RPI::PassFilter::CreateWithPassName(AZ::Name("CopyToSwapChain"), renderPipeline);

                    AZ::RPI::Pass* copyToSwapchainPass = AZ::RPI::PassSystemInterface::Get()->FindFirstPass(passFilter);

                    if (!copyToSwapchainPass)
                    {
                        // Sanity check, can only setup AFR in a pipeline that copies to the SwapChain in the end
                        AZ_Error(
                            "AFRFeatureProcessor",
                            m_afrPipelineName.empty(),
                            "The given RenderPipeline does not have a CopyToSwapChain pass!");
                        return;
                    }

                    // Remember this pipeline as the one AFR is running on
                    m_afrPipelineName = renderPipeline->GetId().GetStringView();
                    m_afrRenderPipeline = { renderPipeline->GetId(), renderPipeline };
                    CollectPassesToSchedule(renderPipeline);
                }

                return;
            }
        case RenderPipelineChangeType::PassChanged:
            {
                if(m_afrSetupState == AFRSetupState::INITIALIZING)
                {
                    // We triggered this update, no need to collect passes -> return
                    m_afrSetupState = AFRSetupState::SETUP_DONE;
                    return;
                }
                else if (renderPipeline->GetId() == m_afrRenderPipeline.first)
                {
                    CollectPassesToSchedule(renderPipeline);
                }

                return;
            }
        case RenderPipelineChangeType::Removed:
            {
                if (renderPipeline->GetId() == m_afrRenderPipeline.first)
                {
                    // Clear CopyPasses
                    m_afrRenderPipeline = { "", nullptr };
                    m_afrCopyPasses.clear();
                    m_scheduledPasses.clear();
                    m_afrSetupState = AFRSetupState::NOT_INITIALIZED;
                    m_afrPipelineName = "";
                }

                return;
            }
        }
    }

    void AFRFeatureProcessor::CollectPassesToSchedule(AZ::RPI::RenderPipeline* renderPipeline)
    {
        if(m_afrSetupState == AFRSetupState::SCHEDULING)
        {
            // To detect valid passes, reset our previous scheduling for now
            for(const auto& pass : m_scheduledPasses)
            {
                pass->SetDeviceIndex(AZ::RHI::MultiDevice::InvalidDeviceIndex);
            }
        }
        m_scheduledPasses.clear();

        AZStd::stack<AZ::RPI::Pass*> stack;
        stack.push(renderPipeline->GetRootPass().get());
        while (!stack.empty())
        {
            auto pass{ stack.top() };
            stack.pop();
            auto parentPass{ pass->AsParent() };

            // If pass is a ParentPass, go over all children
            if (parentPass != nullptr)
            {
                for (auto it{ parentPass->GetChildren().rbegin() }; it != parentPass->GetChildren().rend(); it++)
                {
                    stack.push(it->get());
                }
            }
            else
            {
                // We do not want to schedule the "CopyToSwapChain" pass or any pass that is
                // already scheduled to a fixed device index
                if (!pass->GetName().GetStringView().contains("CopyToSwapChain") &&
                    pass->GetDeviceIndex() == AZ::RHI::MultiDevice::InvalidDeviceIndex)
                {
                    m_scheduledPasses.emplace_back(pass);
                }
            }
        }
    }

    bool IsEnabled(AZ::RPI::Pass* pass)
    {
        if(!pass->IsEnabled())
        {
            return false;
        }
        // Check if parent (or its parent etc.) is enabled
        if (auto parentPass{pass->GetParent()}; parentPass != nullptr)
        {
            if(!parentPass->IsEnabled())
            {
                return false;
            }
            IsEnabled(parentPass);
        }
        return true;
    }

    void AFRFeatureProcessor::OnFrameBegin()
    {
        static int frameNumber{ 0 };

        if (auto& [_, renderPipeline]{ m_afrRenderPipeline }; renderPipeline && renderPipeline->NeedsRender())
        {
            if(m_afrSetupState == AFRSetupState::NOT_INITIALIZED)
            {
                // Add CopyPasses for all devices (except for the first, which is the display GPU)
                for(auto deviceIndex{1}; deviceIndex < m_deviceCount; ++deviceIndex)
                {
                    addAFRCopyPass(renderPipeline, deviceIndex);
                }
                m_afrSetupState = AFRSetupState::INITIALIZING;
                // This will trigger a "OnRenderPipelineChanged::PassChanged" event
                renderPipeline->UpdatePasses();
            }

            auto deviceIndex{ frameNumber % m_deviceCount };
            for (auto pass : m_scheduledPasses)
            {
                if(IsEnabled(pass.get()))
                {
                    pass->SetDeviceIndex(deviceIndex);
                }
            }

            // Enable appropriate CopyPass for deviceIndex and disable rest
            for (auto& [index, copyPass] : m_afrCopyPasses)
            {
                copyPass->SetEnabled(deviceIndex == index);
            }

            m_afrSetupState = AFRSetupState::SCHEDULING;
        }

        ++frameNumber;
    }
    void AFRFeatureProcessor::addAFRCopyPass(AZ::RPI::RenderPipeline* renderPipeline, int deviceIndex)
    {
        if (m_afrCopyPasses.contains(deviceIndex))
        {
            // Already setup
            AZ::RPI::PassFilter passFilter =
                AZ::RPI::PassFilter::CreateWithPassName(AZ::Name("SwapchainMultiDeviceCopyPass"+ AZStd::to_string(deviceIndex)) , renderPipeline);

            AZ::RPI::Pass* pass = AZ::RPI::PassSystemInterface::Get()->FindFirstPass(passFilter);

            if (m_afrRenderPipeline.second != renderPipeline || pass)
            {
                return;
            }
        }

        AZ::RPI::PassFilter passFilter = AZ::RPI::PassFilter::CreateWithPassName(AZ::Name("CopyToSwapChain"), renderPipeline);

        AZ::RPI::Pass* copyToSwapchainPass = AZ::RPI::PassSystemInterface::Get()->FindFirstPass(passFilter);

        if (!copyToSwapchainPass)
        {
            // Sanity check, can only setup AFR in a pipeline that copies to the SwapChain in the end
            return;
        }

        // Add a multi-device CopyPass to copy from deviceIndex to MultiDevice::DefaultDeviceIndex

        AZStd::shared_ptr<AZ::RPI::PassRequest> passRequest = AZStd::make_shared<AZ::RPI::PassRequest>();

        passRequest->m_templateName = AZ::Name("MultiDeviceCopyPassTemplate");

        passRequest->m_passName = AZ::Name("SwapchainMultiDeviceCopyPass" + AZStd::to_string(deviceIndex));

        AZStd::shared_ptr<AZ::RPI::CopyPassData> passData = AZStd::make_shared<AZ::RPI::CopyPassData>();
        passData->m_sourceDeviceIndex = deviceIndex;
        passData->m_destinationDeviceIndex = AZ::RHI::MultiDevice::DefaultDeviceIndex;
        passData->m_cloneInput = false;
        passRequest->m_passData = passData;

        AZ::RPI::PassConnection passConnection;

        passConnection.m_localSlot = AZ::Name{ "InputOutput" };

        // This works as long as this connection is built with a pass request and not a template (which is fine as it isn't done this
        // way for the CopyToSwapChain pass):
        bool found{ false };

        for (auto& connection : copyToSwapchainPass->GetPassDescriptor().m_passRequest->m_connections)
        {
            if (connection.m_localSlot.GetStringView() == AZ::Name{ "Input" }.GetStringView())
            {
                passConnection.m_attachmentRef = connection.m_attachmentRef;
                found = true;
                break;
            }
        }

        AZ_Assert(found, "Connection not found!");

        passRequest->m_connections.emplace_back(passConnection);

        auto afrCopyPass{ AZ::RPI::PassSystemInterface::Get()->CreatePassFromRequest(passRequest.get()) };
        m_afrCopyPasses[deviceIndex] = afrCopyPass;

        auto* parent{ copyToSwapchainPass->GetParent() };
        parent->InsertChild(afrCopyPass, parent->FindChildPassIndex(copyToSwapchainPass->GetName()).GetIndex());
    }
} // namespace AFR
