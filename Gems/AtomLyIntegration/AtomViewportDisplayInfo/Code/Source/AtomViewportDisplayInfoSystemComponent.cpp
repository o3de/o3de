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

#include "AtomViewportDisplayInfoSystemComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <AzCore/Console/IConsole.h>
#include <AzCore/Interface/Interface.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RHI/Factory.h>

#include <CrySystem/MemoryManager.h>
#include <CryCommon/ISystem.h>
#include <CryCommon/IConsole.h>

AZ_CVAR(float, r_fpsInterval, 1.0f, nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
    "The time period over which to calculate the framerate for r_displayInfo");

namespace AZ::Render
{
    static constexpr int DisplayInfoLevelNone = 0;
    static constexpr int DisplayInfoLevelNormal = 1;
    static constexpr int DisplayInfoLevelFull = 2;
    static constexpr int DisplayInfoLevelCompact = 3;

    void AtomViewportDisplayInfoSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AtomViewportDisplayInfoSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<AtomViewportDisplayInfoSystemComponent>("Viewport Display Info", "Manages debug viewport information through r_DisplayInfo")
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

        CrySystemEventBus::Handler::BusConnect();
        AZ::RPI::ViewportContextNotificationBus::Handler::BusConnect(
            AZ::RPI::ViewportContextRequests::Get()->GetDefaultViewportContextName());
    }

    void AtomViewportDisplayInfoSystemComponent::Deactivate()
    {
        AZ::RPI::ViewportContextNotificationBus::Handler::BusDisconnect();
        CrySystemEventBus::Handler::BusDisconnect();
    }

    AZ::RPI::ViewportContextPtr AtomViewportDisplayInfoSystemComponent::GetViewportContext() const
    {
        return AZ::RPI::ViewportContextRequests::Get()->GetDefaultViewportContext();
    }

    void AtomViewportDisplayInfoSystemComponent::DrawLine(AZStd::string_view line, AZ::Color color)
    {
        m_drawParams.m_color = color;
        AzFramework::FontDrawInterface* fontDrawInterface =
            AZ::Interface<AzFramework::FontQueryInterface>::Get()->GetDefaultFontDrawInterface();
        AZ::Vector2 textSize = fontDrawInterface->GetTextSize(m_drawParams, line);
        fontDrawInterface->DrawScreenAlignedText2d(m_drawParams, line);
        m_drawParams.m_position.SetY(m_drawParams.m_position.GetY() + textSize.GetY() + m_lineSpacing);
    }

    void AtomViewportDisplayInfoSystemComponent::OnRenderTick()
    {
        AzFramework::FontDrawInterface* fontDrawInterface =
            AZ::Interface<AzFramework::FontQueryInterface>::Get()->GetDefaultFontDrawInterface();
        AZ::RPI::ViewportContextPtr viewportContext = GetViewportContext();

        if (!fontDrawInterface || !viewportContext || !viewportContext->GetRenderScene())
        {
            return;
        }

        m_fpsInterval = AZStd::chrono::seconds(r_fpsInterval);

        UpdateFramerate();

        if (!m_displayInfoCVar)
        {
            return;
        }
        int displayLevel = m_displayInfoCVar->GetIVal();
        if (displayLevel == DisplayInfoLevelNone)
        {
            return;
        }

        m_drawParams.m_drawViewportId = viewportContext->GetId();
        auto viewportSize = viewportContext->GetViewportSize();
        m_drawParams.m_position = AZ::Vector3(viewportSize.m_width, 0.f, 1.f);
        m_drawParams.m_color = AZ::Colors::White;
        m_drawParams.m_scale = AZ::Vector2(0.7f);
        m_drawParams.m_hAlign = AzFramework::TextHorizontalAlignment::Right;
        m_drawParams.m_monospace = false;
        m_drawParams.m_depthTest = false;
        m_drawParams.m_virtual800x600ScreenSize = true;
        m_drawParams.m_scaleWithWindow = false;
        m_drawParams.m_multiline = true;
        m_drawParams.m_lineSpacing = 0.5f;

        // Calculate line spacing based on the font's actual line height
        const float lineHeight = fontDrawInterface->GetTextSize(m_drawParams, " ").GetY();
        m_lineSpacing = lineHeight * m_drawParams.m_lineSpacing;

        DrawRendererInfo();
        if (displayLevel != DisplayInfoLevelCompact)
        {
            DrawCameraInfo();
            DrawMemoryInfo();
        }
        DrawFramerate();
    }

    void AtomViewportDisplayInfoSystemComponent::OnCrySystemInitialized(ISystem& system, [[maybe_unused]]const SSystemInitParams& initParams)
    {
        m_displayInfoCVar = system.GetGlobalEnvironment()->pConsole->GetCVar("r_DisplayInfo");
    }

    void AtomViewportDisplayInfoSystemComponent::OnCrySystemShutdown([[maybe_unused]]ISystem& system)
    {
        m_displayInfoCVar = nullptr;
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

        auto viewportSize = viewportContext->GetViewportSize();
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

    void AtomViewportDisplayInfoSystemComponent::DrawMemoryInfo()
    {
        static IMemoryManager::SProcessMemInfo processMemInfo;

        // Throttle memory usage updates to avoid potentially expensive memory usage API calls every tick.
        constexpr AZStd::chrono::duration<double> memoryUpdateInterval = AZStd::chrono::seconds(0.5);
        AZStd::chrono::time_point currentTime = m_fpsHistory.back().Get();
        if (m_lastMemoryUpdate.has_value())
        {
            if (currentTime - m_lastMemoryUpdate.value() > memoryUpdateInterval)
            {
                if (auto memoryManager = GetISystem()->GetIMemoryManager())
                {
                    memoryManager->GetProcessMemInfo(processMemInfo);
                }
            }
        }
        m_lastMemoryUpdate = currentTime;


        int peakUsageMB = aznumeric_cast<int>(processMemInfo.PeakPagefileUsage >> 20);
        int currentUsageMB = aznumeric_cast<int>(processMemInfo.PagefileUsage >> 20);
        DrawLine(AZStd::string::format("Mem=%d Peak=%d", currentUsageMB, peakUsageMB));
    }

    void AtomViewportDisplayInfoSystemComponent::UpdateFramerate()
    {
        if (!m_tickRequests)
        {
            m_tickRequests = AZ::TickRequestBus::FindFirstHandler();
        }
        if (!m_tickRequests)
        {
            return;
        }

        AZ::ScriptTimePoint currentTime = m_tickRequests->GetTimeAtCurrentTick();
        // Only keep as much sampling data is is required by our FPS history.
        while (!m_fpsHistory.empty() && (currentTime.Get() - m_fpsHistory.front().Get() > m_fpsInterval))
        {
            m_fpsHistory.pop_front();
        }
        m_fpsHistory.push_back(currentTime);
    }

    void AtomViewportDisplayInfoSystemComponent::DrawFramerate()
    {
        AZStd::chrono::duration<double> actualInterval = AZStd::chrono::seconds(0);
        AZStd::optional<AZ::ScriptTimePoint> lastTime;
        AZStd::optional<double> minFPS;
        AZStd::optional<double> maxFPS;
        for (const AZ::ScriptTimePoint& time : m_fpsHistory)
        {
            if (lastTime.has_value())
            {
                AZStd::chrono::duration<double> deltaTime = time.Get() - lastTime.value().Get();
                if (deltaTime.count() == 0.0)
                {
                    continue;
                }
                double fps = AZStd::chrono::seconds(1) / deltaTime;
                if (!minFPS.has_value())
                {
                    minFPS = fps;
                    maxFPS = fps;
                }
                else
                {
                    minFPS = AZStd::min(minFPS.value(), fps);
                    maxFPS = AZStd::max(maxFPS.value(), fps);
                }
                actualInterval += deltaTime;
            }
            lastTime = time;
        }

        const double averageFPS = aznumeric_cast<double>(m_fpsHistory.size()) / actualInterval.count();
        const double frameIntervalSeconds = m_fpsInterval.count();

        DrawLine(
            AZStd::string::format(
                "FPS %.1f [%.0f..%.0f], frame avg over %.1fs",
                averageFPS,
                minFPS.value_or(0.0),
                maxFPS.value_or(0.0),
                frameIntervalSeconds),
            AZ::Colors::Yellow);
    }
} // namespace AZ::Render
