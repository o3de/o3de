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
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RHI/Factory.h>

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
                        ->Attribute(Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void AtomViewportDisplayInfoSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ViewportDisplayInfoService"));
    }

    void AtomViewportDisplayInfoSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("ViewportDisplayInfoService"));
    }

    void AtomViewportDisplayInfoSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("RPISystem", 0xf2add773));
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
            if (auto rootPass = viewportContext->GetCurrentPipeline()->GetRootPass())
            {
                rootPass->SetPipelineStatisticsQueryEnabled(displayLevel != AtomBridge::ViewportInfoDisplayState::CompactInfo);
                m_updateRootPassQuery = false;
            }
        }

        m_drawParams.m_drawViewportId = viewportContext->GetId();
        auto viewportSize = viewportContext->GetViewportSize();
        m_drawParams.m_position = AZ::Vector3(static_cast<float>(viewportSize.m_width), 0.0f, 1.0f) + AZ::Vector3(r_topRightBorderPadding) * viewportContext->GetDpiScalingFactor();
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
        auto rootPass = viewportContext->GetCurrentPipeline()->GetRootPass();
        const RPI::PipelineStatisticsResult stats = rootPass->GetLatestPipelineStatisticsResult();

        RPI::PassSystemFrameStatistics passSystemFrameStatistics = AZ::RPI::PassSystemInterface::Get()->GetFrameStatistics();

        DrawLine(AZStd::string::format(
            "RenderPasses: %d Vertex Count: %lld Primitive Count: %lld",
            passSystemFrameStatistics.m_numRenderPassesExecuted,
            aznumeric_cast<long long>(stats.m_vertexCount),
            aznumeric_cast<long long>(stats.m_primitiveCount)
        ));
        DrawLine(AZStd::string::format(
            "Total Draw Item Count: %d  Max Draw Items in a Pass: %d",
            passSystemFrameStatistics.m_totalDrawItemsRendered,
            passSystemFrameStatistics.m_maxDrawItemsRenderedInAPass
        ));
    }

    void AtomViewportDisplayInfoSystemComponent::UpdateFramerate()
    {
        auto currentTime = AZStd::chrono::system_clock::now();
        while (!m_fpsHistory.empty() && (currentTime - m_fpsHistory.front()) > m_fpsInterval)
        {
            m_fpsHistory.pop_front();
        }
        m_fpsHistory.push_back(currentTime);
    }

    void AtomViewportDisplayInfoSystemComponent::DrawFramerate()
    {
        AZStd::optional<AZStd::chrono::system_clock::time_point> lastTime;
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

        DrawLine(
            AZStd::string::format(
                "FPS %.1f [%.0f..%.0f], %.1fms/frame, avg over %.1fs",
                averageFPS,
                minFPS,
                maxFPS,
                averageFrameMs,
                frameIntervalSeconds),
            AZ::Colors::Yellow);
    }
} // namespace AZ::Render
