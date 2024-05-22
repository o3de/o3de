/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Console/IConsole.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/Editor/ActionManagerUtils.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorMenuIdentifiers.h>
#include <AzToolsFramework/Viewport/EditorContextMenu.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

AZ_CVAR(
    int,
    ed_contextMenuDisplayThreshold,
    2,
    nullptr,
    AZ::ConsoleFunctorFlags::Null,
    "The minimum 'Manhattan Distance' the mouse can move before the context menu will no longer trigger");

namespace AzToolsFramework
{
    void EditorContextMenuUpdate(EditorContextMenu& contextMenu, const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // could potentially show the context menu
        if (mouseInteraction.m_mouseInteraction.m_mouseButtons.Right() &&
            mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::Down)
        {
            contextMenu.m_clickPoint =
                ViewportInteraction::QPointFromScreenPoint(mouseInteraction.m_mouseInteraction.m_mousePick.m_screenCoordinates);
        }

        // do show the context menu
        if (mouseInteraction.m_mouseInteraction.m_mouseButtons.Right() &&
            mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::Up)
        {
            const QPoint currentScreenCoords =
                ViewportInteraction::QPointFromScreenPoint(mouseInteraction.m_mouseInteraction.m_mousePick.m_screenCoordinates);

            // if the mouse hasn't moved, open the pop-up menu
            if ((currentScreenCoords - contextMenu.m_clickPoint).manhattanLength() < ed_contextMenuDisplayThreshold &&
                !mouseInteraction.m_captured)
            {
                auto menuManagerInterface = AZ::Interface<MenuManagerInterface>::Get();
                if (menuManagerInterface)
                {
                    menuManagerInterface->DisplayMenuUnderCursor(EditorIdentifiers::ViewportContextMenuIdentifier);
                }
            }
        }
    }
} // namespace AzToolsFramework
