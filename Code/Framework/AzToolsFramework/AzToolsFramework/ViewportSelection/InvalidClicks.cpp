/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Console/Console.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/Viewport/ViewportTypes.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <AzToolsFramework/ViewportSelection/InvalidClicks.h>

AZ_CVAR(float, ed_invalidClickRadius, 10.0f, nullptr, AZ::ConsoleFunctorFlags::Null, "Maximum invalid click radius to expand to");
AZ_CVAR(float, ed_invalidClickDuration, 1.0f, nullptr, AZ::ConsoleFunctorFlags::Null, "Duration to display the invalid click feedback");
AZ_CVAR(float, ed_invalidClickMessageSize, 0.8f, nullptr, AZ::ConsoleFunctorFlags::Null, "Size of text for invalid message");
AZ_CVAR(
    float,
    ed_invalidClickMessageVerticalOffset,
    30.0f,
    nullptr,
    AZ::ConsoleFunctorFlags::Null,
    "Vertical offset from cursor of invalid click message");

namespace AzToolsFramework
{
    void ExpandingFadingCircles::Begin(const AzFramework::ScreenPoint& screenPoint)
    {
        FadingCircle fadingCircle;
        fadingCircle.m_position = screenPoint;
        fadingCircle.m_opacity = 1.0f;
        fadingCircle.m_radius = 0.0f;
        m_fadingCircles.push_back(fadingCircle);
    }

    void ExpandingFadingCircles::Update(const float deltaTime)
    {
        for (auto& fadingCircle : m_fadingCircles)
        {
            fadingCircle.m_opacity = AZStd::max(fadingCircle.m_opacity - (deltaTime / ed_invalidClickDuration), 0.0f);
            fadingCircle.m_radius += deltaTime * ed_invalidClickRadius;
        }

        m_fadingCircles.erase(
            AZStd::remove_if(
                m_fadingCircles.begin(), m_fadingCircles.end(),
                [](const FadingCircle& fadingCircle)
                {
                    return fadingCircle.m_opacity <= 0.0f;
                }),
            m_fadingCircles.end());
    }

    bool ExpandingFadingCircles::Updating()
    {
        return !m_fadingCircles.empty();
    }

    void ExpandingFadingCircles::Display(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        const AZ::Vector2 viewportSize = AzToolsFramework::GetCameraState(viewportInfo.m_viewportId).m_viewportSize;

        for (const auto& fadingCircle : m_fadingCircles)
        {
            const auto position = AzFramework::Vector2FromScreenPoint(fadingCircle.m_position) / viewportSize;
            debugDisplay.SetColor(AZ::Color(1.0f, 1.0f, 1.0f, fadingCircle.m_opacity));
            debugDisplay.DrawWireCircle2d(position, fadingCircle.m_radius * 0.005f, 0.0f);
        }
    }

    void FadingText::Begin(const AzFramework::ScreenPoint& screenPoint)
    {
        m_opacity = 1.0f;
        m_invalidClickPosition = screenPoint;
    }

    void FadingText::Update(const float deltaTime)
    {
        m_opacity -= deltaTime / ed_invalidClickDuration;
    }

    bool FadingText::Updating()
    {
        return m_opacity >= 0.0f;
    }

    void FadingText::Display(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        if (constexpr float MinOpacity = 0.05f; m_opacity >= MinOpacity)
        {
            debugDisplay.SetColor(AZ::Color(1.0f, 1.0f, 1.0f, m_opacity));
            debugDisplay.Draw2dTextLabel(
                aznumeric_cast<float>(m_invalidClickPosition.m_x),
                aznumeric_cast<float>(m_invalidClickPosition.m_y) - ed_invalidClickMessageVerticalOffset, ed_invalidClickMessageSize,
                m_message.c_str(), true);
        }
    }

    void InvalidClicks::AddInvalidClick(const AzFramework::ScreenPoint& screenPoint)
    {
        AZ::TickBus::Handler::BusConnect();

        for (auto& invalidClickBehavior : m_invalidClickBehaviors)
        {
            invalidClickBehavior->Begin(screenPoint);
        }
    }

    void InvalidClicks::OnTick(const float deltaTime, [[maybe_unused]] const AZ::ScriptTimePoint time)
    {
        for (auto& invalidClickBehavior : m_invalidClickBehaviors)
        {
            invalidClickBehavior->Update(deltaTime);
        }

        const auto updating = AZStd::any_of(
            m_invalidClickBehaviors.begin(), m_invalidClickBehaviors.end(),
            [](const auto& invalidClickBehavior)
            {
                return invalidClickBehavior->Updating();
            });

        if (!updating && AZ::TickBus::Handler::BusIsConnected())
        {
            AZ::TickBus::Handler::BusDisconnect();
        }
    }

    void InvalidClicks::Display2d(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        debugDisplay.DepthTestOff();

        for (const auto& invalidClickBehavior : m_invalidClickBehaviors)
        {
            invalidClickBehavior->Display(viewportInfo, debugDisplay);
        }

        debugDisplay.DepthTestOn();
    }
} // namespace AzToolsFramework
