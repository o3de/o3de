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

#include "EditorContextMenu.h"

#include "AzToolsFramework/Viewport/ViewportMessages.h"

namespace AzToolsFramework
{
    void EditorContextMenuUpdate(EditorContextMenu& contextMenu, const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        // could potentially show the context menu
        if (mouseInteraction.m_mouseInteraction.m_mouseButtons.Right() &&
            mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::Down)
        {
            contextMenu.m_shouldOpen = true;
            contextMenu.m_clickPoint =
                ViewportInteraction::QPointFromScreenPoint(mouseInteraction.m_mouseInteraction.m_mousePick.m_screenCoordinates);
        }

        // disable shouldOpen if right clicking an moving the mouse
        if (mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::Move)
        {
            const QPoint currentScreenCoords =
                ViewportInteraction::QPointFromScreenPoint(mouseInteraction.m_mouseInteraction.m_mousePick.m_screenCoordinates);

            contextMenu.m_shouldOpen = contextMenu.m_shouldOpen && (currentScreenCoords - contextMenu.m_clickPoint).manhattanLength() < 2;
        }

        // do show the context menu
        if (mouseInteraction.m_mouseInteraction.m_mouseButtons.Right() &&
            mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::Up)
        {
            if (contextMenu.m_shouldOpen)
            {
                QWidget* parent = nullptr;
                ViewportInteraction::MainEditorViewportInteractionRequestBus::EventResult(
                    parent, mouseInteraction.m_mouseInteraction.m_interactionId.m_viewportId,
                    &ViewportInteraction::MainEditorViewportInteractionRequestBus::Events::GetWidgetForViewportContextMenu);

                contextMenu.m_menu = new QMenu(parent);
                contextMenu.m_menu->setAttribute(Qt::WA_DeleteOnClose);
                contextMenu.m_menu->setParent(parent);

                // Populate global context menu.
                const int contextMenuFlag = 0;
                EditorEvents::Bus::BroadcastReverse(
                    &EditorEvents::PopulateEditorGlobalContextMenu, contextMenu.m_menu.data(),
                    AzFramework::Vector2FromScreenPoint(mouseInteraction.m_mouseInteraction.m_mousePick.m_screenCoordinates),
                    contextMenuFlag);

                if (!contextMenu.m_menu->isEmpty())
                {
                    // Use popup instead of exec; this avoids blocking input event processing while the menu dialog is active
                    contextMenu.m_menu->popup(QCursor::pos());
                }
            }
        }
    }
} // namespace AzToolsFramework
