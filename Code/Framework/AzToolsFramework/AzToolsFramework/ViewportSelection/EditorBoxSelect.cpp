/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorBoxSelect.h"

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

#include <QApplication>

namespace AzToolsFramework
{
    static const AZ::Color s_boxSelectColor = AZ::Color(1.0f, 1.0f, 1.0f, 0.4f);
    static const float s_boxSelectLineWidth = 2.0f;

    EditorBoxSelect::EditorBoxSelect()
    {
        // discard double click interval as box select is only interested in 'move' detection
        // note: this also simplifies integration tests that do not have delays between presses
        m_clickDetector.SetDoubleClickInterval(0.0f);
    }

    void EditorBoxSelect::HandleMouseInteraction(const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::Down)
        {
            m_cursorPositionAtDownEvent = mouseInteraction.m_mouseInteraction.m_mousePick.m_screenCoordinates;
        }

        m_cursorState.SetCurrentPosition(mouseInteraction.m_mouseInteraction.m_mousePick.m_screenCoordinates);

        const auto selectClickEvent = ClickDetectorEventFromViewportInteraction(mouseInteraction);
        const auto clickOutcome = m_clickDetector.DetectClick(selectClickEvent, m_cursorState.CursorDelta());
        if (clickOutcome == AzFramework::ClickDetector::ClickOutcome::Move)
        {
            if (m_leftMouseDown)
            {
                m_leftMouseDown(mouseInteraction);
            }

            m_boxSelectRegion = QRect{ ViewportInteraction::QPointFromScreenPoint(m_cursorPositionAtDownEvent), QSize{ 0, 0 } };
        }

        if (m_boxSelectRegion)
        {
            if (mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::Move)
            {
                m_boxSelectRegion->setWidth(
                    mouseInteraction.m_mouseInteraction.m_mousePick.m_screenCoordinates.m_x - m_boxSelectRegion->x());
                m_boxSelectRegion->setHeight(
                    mouseInteraction.m_mouseInteraction.m_mousePick.m_screenCoordinates.m_y - m_boxSelectRegion->y());

                if (m_mouseMove)
                {
                    m_mouseMove(mouseInteraction);
                }
            }

            if (clickOutcome == AzFramework::ClickDetector::ClickOutcome::Release)
            {
                if (m_leftMouseUp)
                {
                    m_leftMouseUp();
                }

                m_boxSelectRegion.reset();
            }
        }

        m_previousModifiers = mouseInteraction.m_mouseInteraction.m_keyboardModifiers;
    }

    void EditorBoxSelect::Display2d(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        m_cursorState.Update();

        if (m_boxSelectRegion)
        {
            debugDisplay.DepthTestOff();
            debugDisplay.SetLineWidth(s_boxSelectLineWidth);
            debugDisplay.SetColor(s_boxSelectColor);

            AZ::Vector2 viewportSize = AzToolsFramework::GetCameraState(viewportInfo.m_viewportId).m_viewportSize;

            debugDisplay.DrawWireQuad2d(
                AZ::Vector2(aznumeric_cast<float>(m_boxSelectRegion->x()), aznumeric_cast<float>(m_boxSelectRegion->y())) / viewportSize,
                AZ::Vector2(
                    aznumeric_cast<float>(m_boxSelectRegion->x()) + aznumeric_cast<float>(m_boxSelectRegion->width()),
                    aznumeric_cast<float>(m_boxSelectRegion->y()) + aznumeric_cast<float>(m_boxSelectRegion->height())) /
                    viewportSize,
                0.f);

            debugDisplay.DepthTestOn();

            m_previousModifiers = AzToolsFramework::ViewportInteraction::QueryKeyboardModifiers();
        }
    }

    void EditorBoxSelect::DisplayScene(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        if (m_displayScene)
        {
            m_displayScene(viewportInfo, debugDisplay);
        }
    }

    void EditorBoxSelect::InstallLeftMouseDown(
        const AZStd::function<void(const ViewportInteraction::MouseInteractionEvent& mouseInteraction)>& leftMouseDown)
    {
        m_leftMouseDown = leftMouseDown;
    }

    void EditorBoxSelect::InstallMouseMove(
        const AZStd::function<void(const ViewportInteraction::MouseInteractionEvent& mouseInteraction)>& mouseMove)
    {
        m_mouseMove = mouseMove;
    }

    void EditorBoxSelect::InstallLeftMouseUp(const AZStd::function<void()>& leftMouseUp)
    {
        m_leftMouseUp = leftMouseUp;
    }

    void EditorBoxSelect::InstallDisplayScene(
        const AZStd::function<void(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)>&
            displayScene)
    {
        m_displayScene = displayScene;
    }
} // namespace AzToolsFramework
