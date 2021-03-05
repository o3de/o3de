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

#include <QApplication>

namespace AzToolsFramework
{
    static const AZ::Color s_boxSelectColor = AZ::Color(1.0f, 1.0f, 1.0f, 0.4f);
    static const float s_boxSelectLineWidth = 2.0f;

    void EditorBoxSelect::HandleMouseInteraction(
        const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (mouseInteraction.m_mouseInteraction.m_mouseButtons.Left() &&
            mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::Down)
        {
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
                m_boxSelectRegion->setWidth(
                    mouseInteraction.m_mouseInteraction.m_mousePick.m_screenCoordinates.m_x - m_boxSelectRegion->x());
                m_boxSelectRegion->setHeight(
                    mouseInteraction.m_mouseInteraction.m_mousePick.m_screenCoordinates.m_y - m_boxSelectRegion->y());

                if (m_mouseMove)
                {
                    m_mouseMove(mouseInteraction);
                }
            }

            if (mouseInteraction.m_mouseInteraction.m_mouseButtons.Left() &&
                mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::Up)
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

    void EditorBoxSelect::Display2d(AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (m_boxSelectRegion)
        {
            debugDisplay.DepthTestOff();
            debugDisplay.SetLineWidth(s_boxSelectLineWidth);
            debugDisplay.SetColor(s_boxSelectColor);

            debugDisplay.DrawWireBox(
                AZ::Vector3(
                    static_cast<float>(m_boxSelectRegion->x()), static_cast<float>(m_boxSelectRegion->y()), 0.0f),
                AZ::Vector3(
                    static_cast<float>(m_boxSelectRegion->x()) + static_cast<float>(m_boxSelectRegion->width()),
                    static_cast<float>(m_boxSelectRegion->y()) + static_cast<float>(m_boxSelectRegion->height()), 0.0f));

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
