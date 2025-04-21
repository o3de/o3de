/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AtomViewportDisplayInfoSystemComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <AzCore/Console/IConsole.h>
#include <AzCore/Interface/Interface.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Image/StreamingImagePool.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/RHIMemoryStatisticsInterface.h>
#include <Atom/RHI.Reflect/MemoryUsage.h>

#include <CryCommon/ISystem.h>
#include <CryCommon/IConsole.h>

namespace AZ::Render
{
    AZ_CVAR(int, r_displayInfo, 1, [](const int& newDisplayInfoVal)->void
        {
            // Forward this event to the system component so it can update accordingly.
            // This callback only gets triggered by console commands, so this will not recurse.
            AtomBridge::AtomViewportInfoDisplayRequestBus::Broadcast(
                &AtomBridge::AtomViewportInfoDisplayRequestBus::Events::SetDisplayState,
                aznumeric_cast<AtomBridge::ViewportInfoDisplayState>(newDisplayInfoVal)
            );
        }, AZ::ConsoleFunctorFlags::DontReplicate,
        "Toggles debugging information display.\n"
        "Usage: r_displayInfo [0=off/1=show/2=enhanced/3=compact]"
    );
    AZ_CVAR(float, r_fpsCalcInterval, 1.0f, nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
        "The time period over which to calculate the framerate for r_displayInfo."
    );

    AZ_CVAR(
        AZ::Vector2, r_topRightBorderPadding, AZ::Vector2(-40.0f, 22.0f), nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
        "The top right border padding for the viewport debug display text");

    void AtomViewportDisplayInfoSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AtomViewportDisplayInfoSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<AtomViewportDisplayInfoSystemComponent>("Viewport Display Info", "Manages debug viewport information through r_displayInfo")
                    ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void AtomViewportDisplayInfoSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ViewportDisplayInfoService"));
    }

    void AtomViewportDisplayInfoSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("ViewportDisplayInfoService"));
    }

    void AtomViewportDisplayInfoSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("RPISystem"));
    }

    void AtomViewportDisplayInfoSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void AtomViewportDisplayInfoSystemComponent::Activate()
    {
        AZ::Name apiName = AZ::RHI::Factory::Get().GetName();
        if (!apiName.IsEmpty())
        {
            m_rendererDescription = AZStd::string::format("Atom using %s RHI", apiName.GetCStr());
        }

        AZ::RPI::ViewportContextNotificationBus::Handler::BusConnect(
            AZ::RPI::ViewportContextRequests::Get()->GetDefaultViewportContextName());
        AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::Handler::BusConnect();
    }

    void AtomViewportDisplayInfoSystemComponent::Deactivate()
    {
        AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::Handler::BusDisconnect();
        AZ::RPI::ViewportContextNotificationBus::Handler::BusDisconnect();
    }

    AZ::RPI::ViewportContextPtr AtomViewportDisplayInfoSystemComponent::GetViewportContext() const
    {
        return AZ::RPI::ViewportContextRequests::Get()->GetDefaultViewportContext();
    }

    void AtomViewportDisplayInfoSystemComponent::DrawLine(AZStd::string_view line, AZ::Color color)
    {
        m_drawParams.m_color = color;
        AZ::Vector2 textSize = m_fontDrawInterface->GetTextSize(m_drawParams, line);
        m_fontDrawInterface->DrawScreenAlignedText2d(m_drawParams, line);
        m_drawParams.m_position.SetY(m_drawParams.m_position.GetY() + textSize.GetY() + m_lineSpacing);
    }

    void AtomViewportDisplayInfoSystemComponent::OnRenderTick()
    {
        if (!m_fontDrawInterface)
        {
            auto fontQueryInterface = AZ::Interface<AzFramework::FontQueryInterface>::Get();
            if (!fontQueryInterface)
            {
                return;
            }
            m_fontDrawInterface =
                fontQueryInterface->GetDefaultFontDrawInterface();
        }
        AZ::RPI::ViewportContextPtr viewportContext = GetViewportContext();

        if (!m_fontDrawInterface || !viewportContext || !viewportContext->GetRenderScene() ||
            !AZ::Interface<AzFramework::FontQueryInterface>::Get())
        {
            return;
        }

        m_fpsInterval = AZStd::chrono::seconds(static_cast<AZStd::sys_time_t>(r_fpsCalcInterval));

        UpdateFramerate();

        const AtomBridge::ViewportInfoDisplayState displayLevel = GetDisplayState();
        if (displayLevel == AtomBridge::ViewportInfoDisplayState::NoInfo)
        {
            return;
        }

        if (m_updateRootPassQuery)
        {
            if (auto currentPipeline = viewportContext->GetCurrentPipeline())
            {
                if (auto rootPass = currentPipeline->GetRootPass())
                {
                    rootPass->SetPipelineStatisticsQueryEnabled(displayLevel != AtomBridge::ViewportInfoDisplayState::CompactInfo);
                    m_updateRootPassQuery = false;
                }
            }
        }

        m_drawParams.m_drawViewportId = viewportContext->GetId();
        
        auto viewportSize = viewportContext->GetViewportSize();
        m_drawParams.m_position = AZ::Vector3(static_cast<float>(viewportSize.m_width), 0.0f, 1.0f) +
            AZ::Vector3(r_topRightBorderPadding) * viewportContext->GetDpiScalingFactor();
        
        m_drawParams.m_color = AZ::Colors::White;
        m_drawParams.m_scale = AZ::Vector2(BaseFontSize);
        m_drawParams.m_hAlign = AzFramework::TextHorizontalAlignment::Right;
        m_drawParams.m_monospace = false;
        m_drawParams.m_depthTest = false;
        m_drawParams.m_virtual800x600ScreenSize = false;
        m_drawParams.m_scaleWithWindow = false;
        m_drawParams.m_multiline = true;
        m_drawParams.m_lineSpacing = 0.5f;

        // Calculate line spacing based on the font's actual line height
        const float lineHeight = m_fontDrawInterface->GetTextSize(m_drawParams, " ").GetY();
        m_lineSpacing = lineHeight * m_drawParams.m_lineSpacing;

        DrawRendererInfo();
        if (displayLevel == AtomBridge::ViewportInfoDisplayState::FullInfo)
        {
            DrawCameraInfo();
        }
        if (displayLevel != AtomBridge::ViewportInfoDisplayState::CompactInfo)
        {
            DrawPassInfo();
        }
        DrawMemoryInfo();
        DrawFramerate();
    }

    AtomBridge::ViewportInfoDisplayState AtomViewportDisplayInfoSystemComponent::GetDisplayState() const
    {
        return aznumeric_cast<AtomBridge::ViewportInfoDisplayState>(r_displayInfo.operator int());
    }

    void AtomViewportDisplayInfoSystemComponent::SetDisplayState(AtomBridge::ViewportInfoDisplayState state)
    {
        r_displayInfo = aznumeric_cast<int>(state);
        AtomBridge::AtomViewportInfoDisplayNotificationBus::Broadcast(
            &AtomBridge::AtomViewportInfoDisplayNotificationBus::Events::OnViewportInfoDisplayStateChanged,
            state);
        m_updateRootPassQuery = true;
    }

    void AtomViewportDisplayInfoSystemComponent::DrawRendererInfo()
    {
        DrawLine(m_rendererDescription, AZ::Colors::Yellow);

        // resolution and MSAA state
        AZ::RPI::ViewportContextPtr viewportContext = GetViewportContext();
        const RHI::MultisampleState& multisampleState = RPI::RPISystemInterface::Get()->GetApplicationMultisampleState();

        AZ::RPI::ScenePtr pScene = viewportContext->GetRenderScene();
       
        AZStd::string defaultAA = "MSAA";
        bool hasAAMethod = false;
        if (pScene != nullptr)
        {
            AZ::RPI::RenderPipelinePtr pPipeline = pScene->GetDefaultRenderPipeline();
 
            AZ::RPI::AntiAliasingMode defaultAAMethod = pPipeline->GetActiveAAMethod();
            defaultAA = AZ::RPI::RenderPipeline::GetAAMethodNameByIndex(defaultAAMethod);
            hasAAMethod = (defaultAAMethod != AZ::RPI::AntiAliasingMode::MSAA && defaultAAMethod != AZ::RPI::AntiAliasingMode::Default);
        }
        auto resolutionStr =
            AZStd::string::format(
                "Resolution: %dx%d", viewportContext->GetViewportSize().m_width, viewportContext->GetViewportSize().m_height);
        auto msaaStr =
            multisampleState.m_samples > 1 ? AZStd::string::format("MSAA %dx", multisampleState.m_samples) : AZStd::string("NoMSAA");
 
        if (hasAAMethod)
        {
            if (multisampleState.m_samples > 1)
            {
                DrawLine(AZStd::string::format("%s (%s + %s)", resolutionStr.c_str(), defaultAA.c_str(), msaaStr.c_str()));
            }
            else
            {
                DrawLine(AZStd::string::format("%s (%s)", resolutionStr.c_str(), defaultAA.c_str()));
            }
        }
        else
        {
            DrawLine(AZStd::string::format("%s (%s)", resolutionStr.c_str(), msaaStr.c_str()));
        }

        if(viewportContext->GetCurrentPipeline())   // avoid VR crash on nullptr
        {
            DrawLine(AZStd::string::format("Render pipeline: %s", viewportContext->GetCurrentPipeline()->GetId().GetCStr()));
        }
    }

    void AtomViewportDisplayInfoSystemComponent::DrawCameraInfo()
    {
        AZ::RPI::ViewportContextPtr viewportContext = GetViewportContext();
        AZ::RPI::ViewPtr currentView = viewportContext->GetDefaultView();
        if (currentView == nullptr)
        {
            return;
        }

        AzFramework::CameraState cameraState;
        AzFramework::SetCameraClippingVolumeFromPerspectiveFovMatrixRH(cameraState, currentView->GetViewToClipMatrix());
        const AZ::Transform transform = currentView->GetCameraTransform();
        const AZ::Vector3 translation = transform.GetTranslation();
        const AZ::Vector3 rotation = transform.GetEulerDegrees();
        DrawLine(AZStd::string::format(
            "CamPos=%.2f %.2f %.2f Angl=%3.0f %3.0f %4.0f ZN=%.2f ZF=%.0f",
            translation.GetX(), translation.GetY(), translation.GetZ(),
            rotation.GetX(), rotation.GetY(), rotation.GetZ(),
            cameraState.m_nearClip, cameraState.m_farClip
        ));
    }

    void AtomViewportDisplayInfoSystemComponent::DrawPassInfo()
    {
        AZ::RPI::ViewportContextPtr viewportContext = GetViewportContext();
        if (!viewportContext->GetCurrentPipeline())
        {
            return;
        }
        auto rootPass = viewportContext->GetCurrentPipeline()->GetRootPass();

        RPI::PassSystemFrameStatistics passSystemFrameStatistics = AZ::RPI::PassSystemInterface::Get()->GetFrameStatistics();

        DrawLine(AZStd::string::format(
            "RenderPasses: %d",
            passSystemFrameStatistics.m_numRenderPassesExecuted
        ));
        DrawLine(AZStd::string::format(
            "Total Draw Item Count: %d  Max Draw Items in a Pass: %d",
            passSystemFrameStatistics.m_totalDrawItemsRendered,
            passSystemFrameStatistics.m_maxDrawItemsRenderedInAPass
        ));
    }

    void AtomViewportDisplayInfoSystemComponent::UpdateFramerate()
    {
        auto currentTime = AZStd::chrono::steady_clock::now();
        while (!m_fpsHistory.empty() && (currentTime - m_fpsHistory.front()) > m_fpsInterval)
        {
            m_fpsHistory.pop_front();
        }
        m_fpsHistory.push_back(currentTime);
    }

    void AtomViewportDisplayInfoSystemComponent::DrawMemoryInfo()
    {
        RHI::RHISystemInterface* rhi = RHI::RHISystemInterface::Get();
        if (!rhi)
        {
            return;
        }

        RHI::RHIMemoryStatisticsInterface* rhiMemStats = RHI::RHIMemoryStatisticsInterface::Get();
        if (!rhiMemStats)
        {
            return;
        }

        const RHI::MemoryStatistics* stats = rhiMemStats->GetMemoryStatistics();
        if (!stats)
        {
            return;
        }

        // Accumulate total device memory pressure (reserved, resident)
        size_t deviceResident = 0;
        size_t deviceReserved = 0;

        for (const auto& heap : stats->m_heaps)
        {
            if (heap.m_heapMemoryType == RHI::HeapMemoryLevel::Device)
            {
                deviceResident += heap.m_memoryUsage.m_usedResidentInBytes;                
                deviceReserved += heap.m_memoryUsage.m_totalResidentInBytes;
            }
        }

        // Fallback if the information from the heaps is not available, we use the pools instead.
        if (deviceResident == 0 || deviceReserved == 0)
        {
            deviceResident = deviceReserved = 0;
            for (const auto& pool : stats->m_pools)
            {
                deviceReserved += pool.m_memoryUsage.GetHeapMemoryUsage(RHI::HeapMemoryLevel::Device).m_totalResidentInBytes;
                deviceResident += pool.m_memoryUsage.GetHeapMemoryUsage(RHI::HeapMemoryLevel::Device).m_usedResidentInBytes;
            }
        }

        // Query for available device memory
        float availableDeviceMemoryMB = 0.f;

        static constexpr size_t MB = 1u << 20;

        if (RHI::Device* device = rhi->GetDevice(); device)
        {
            const RHI::PhysicalDeviceDescriptor& deviceDesc = device->GetPhysicalDevice().GetDescriptor();
            availableDeviceMemoryMB =
                static_cast<float>(deviceDesc.m_heapSizePerLevel[static_cast<size_t>(RHI::HeapMemoryLevel::Device)]) / MB;
        }

        float deviceResidentMB = static_cast<float>(deviceResident) / MB;
        float deviceReservedMB = static_cast<float>(deviceReserved) / MB;

        AZ::Color deviceMemoryColor = AZ::Colors::White;
        if (availableDeviceMemoryMB != 0.f)
        {
            // Highlight text based on device memory pressure
            if (deviceResidentMB > 0.6f * availableDeviceMemoryMB)
            {
                deviceMemoryColor = AZ::Colors::Yellow;
            }
            else if (deviceResidentMB > 0.8f * availableDeviceMemoryMB)
            {
                deviceMemoryColor = AZ::Colors::Red;
            }
        }
        DrawLine(
            AZStd::string::format(
                "VRAM (resident/reserved): %.2f / %.2f MiB | %.2f available", deviceResidentMB, deviceReservedMB, availableDeviceMemoryMB),
            deviceMemoryColor);

        // RPI default StreamingImagePool usage
        Data::Instance<RPI::StreamingImagePool> streamingImagePool = RPI::ImageSystemInterface::Get()->GetSystemStreamingPool();
        const RHI::HeapMemoryUsage& imagePoolMemoryUsage = streamingImagePool->GetRHIPool()->GetHeapMemoryUsage(RHI::HeapMemoryLevel::Device);

        float imagePoolUsedAllocatedMB = static_cast<float>(imagePoolMemoryUsage.m_usedResidentInBytes) / MB;
        float imagePoolTotalAllocatedMB = static_cast<float>(imagePoolMemoryUsage.m_totalResidentInBytes) / MB;
        float imagePoolBudgetMB = static_cast<float>(imagePoolMemoryUsage.m_budgetInBytes) / MB;
        bool supportTiledImage = streamingImagePool->GetRHIPool()->SupportTiledImage();
        AZ::Color fontColor = AZ::Colors::White;
        if (streamingImagePool->IsMemoryLow())
        {
            fontColor = AZ::Colors::Red;
        }

        DrawLine(
            AZStd::string::format("Texture %s (used/allocated/budget): %.2f / %.2f/%.2f MiB", supportTiledImage?"Tiled":"", imagePoolUsedAllocatedMB, imagePoolTotalAllocatedMB, imagePoolBudgetMB),
            fontColor
        );
    }

    void AtomViewportDisplayInfoSystemComponent::DrawFramerate()
    {
        AZStd::optional<AZStd::chrono::steady_clock::time_point> lastTime;
        double minFPS = DBL_MAX;
        double maxFPS = 0;
        AZStd::chrono::duration<double> deltaTime;
        for (const auto& time : m_fpsHistory)
        {
            if (lastTime.has_value())
            {
                deltaTime = time - lastTime.value();
                double fps = AZStd::chrono::seconds(1) / deltaTime;
                minFPS = AZStd::min(minFPS, fps);
                maxFPS = AZStd::max(maxFPS, fps);
            }
            lastTime = time;
        }

        double averageFPS = 0;
        double averageFrameMs = 0;
        if (m_fpsHistory.size() > 1)
        {
            deltaTime = m_fpsHistory.back() - m_fpsHistory.front();
            averageFPS = AZStd::chrono::seconds(m_fpsHistory.size()) / deltaTime;
            averageFrameMs = 1000.0f/averageFPS;
        }

        const double frameIntervalSeconds = m_fpsInterval.count();

        auto ClampedFloatDisplay = [](double value, const char* format) -> AZStd::string
        {
            constexpr float upperLimit = 10000.0f;
            return value > upperLimit ? "inf" : AZStd::string::format(format, value);
        };

        DrawLine(
            AZStd::string::format(
                "FPS %s [%s..%s], %sms/frame, avg over %.1fs",
                ClampedFloatDisplay(averageFPS, "%.1f").c_str(),
                ClampedFloatDisplay(minFPS, "%.0f").c_str(),
                ClampedFloatDisplay(maxFPS, "%.0f").c_str(),
                ClampedFloatDisplay(averageFrameMs, "%.1f").c_str(),
                frameIntervalSeconds),
            AZ::Colors::Yellow);
    }
} // namespace AZ::Render
