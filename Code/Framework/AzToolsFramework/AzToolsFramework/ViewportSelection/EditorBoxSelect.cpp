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

#include "EditorBoxSelect.h"

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

#include <QApplication>

#pragma optimize("", off)
#pragma inline_depth(0)

namespace AzToolsFramework
{
    static const AZ::Color s_boxSelectColor = AZ::Color(1.0f, 1.0f, 1.0f, 0.4f);
    static const float s_boxSelectLineWidth = 2.0f;

    EditorBoxSelect::EditorBoxSelect()
    {
        m_clickDetector.SetDeadZone(4.0f);
        m_clickDetector.m_debugName = "BoxSelect";
    }

    void EditorBoxSelect::HandleMouseInteraction(
        const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        m_cursorState.SetCurrentPosition(mouseInteraction.m_mouseInteraction.m_mousePick.m_screenCoordinates);

        const AzFramework::ClickDetector::ClickEvent selectClickEvent = [&mouseInteraction] {
            if (mouseInteraction.m_mouseInteraction.m_mouseButtons.Left())
            {
                if (mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::Down)
                {
                    return AzFramework::ClickDetector::ClickEvent::Down;
                }

                if (mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::Up)
                {
                    return AzFramework::ClickDetector::ClickEvent::Up;
                }
            }
            return AzFramework::ClickDetector::ClickEvent::Nil;
        }();

        const auto clickOutcome = m_clickDetector.DetectClick(selectClickEvent, m_cursorState.CursorDelta());
        if (clickOutcome == AzFramework::ClickDetector::ClickOutcome::Move)
        //if (mouseInteraction.m_mouseInteraction.m_mouseButtons.Left() &&
        //    mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::Down)
        {
            AZ_Printf("AzToolsFramework", "Box Select - Begin");

            if (m_leftMouseDown)
            {
                m_leftMouseDown(mouseInteraction);
            }

            m_boxSelectRegion = QRect
            {
                ViewportInteraction::QPointFromScreenPoint(
                    mouseInteraction.m_mouseInteraction.m_mousePick.m_screenCoordinates),
                QSize { 0, 0 }
            };
        }

        if (m_boxSelectRegion)
        {
            if (mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::Move)
            {
                AZ_Printf("AzToolsFramework", "Box Select - Continue");

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
            //if (mouseInteraction.m_mouseInteraction.m_mouseButtons.Left() &&
            //    mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::Up)
            {
                AZ_Printf("AzToolsFramework", "Box Select - End");

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
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        m_cursorState.Update();

        if (m_boxSelectRegion)
        {
            debugDisplay.DepthTestOff();
            debugDisplay.SetLineWidth(s_boxSelectLineWidth);
            debugDisplay.SetColor(s_boxSelectColor);

            AZ::Vector2 viewportSize = AzToolsFramework::GetCameraState(viewportInfo.m_viewportId).m_viewportSize;

            debugDisplay.DrawWireQuad2d(
                AZ::Vector2(
                    aznumeric_cast<float>(m_boxSelectRegion->x()), aznumeric_cast<float>(m_boxSelectRegion->y())) / viewportSize,
                AZ::Vector2(
                    aznumeric_cast<float>(m_boxSelectRegion->x()) + aznumeric_cast<float>(m_boxSelectRegion->width()),
                    aznumeric_cast<float>(m_boxSelectRegion->y()) + aznumeric_cast<float>(m_boxSelectRegion->height())) / viewportSize,
                0.f);

            debugDisplay.DepthTestOn();

            m_previousModifiers = ViewportInteraction::KeyboardModifiers(
                ViewportInteraction::TranslateKeyboardModifiers(QApplication::queryKeyboardModifiers()));
        }
    }

    void EditorBoxSelect::DisplayScene(
        const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
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

    void EditorBoxSelect::InstallLeftMouseUp(
        const AZStd::function<void()>& leftMouseUp)
    {
        m_leftMouseUp = leftMouseUp;
    }

    void EditorBoxSelect::InstallDisplayScene(
        const AZStd::function<void(const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay)>& displayScene)
    {
        m_displayScene = displayScene;
    }
} // namespace AzToolsFramework
