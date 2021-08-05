/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QMenu>
#include <QPoint>
#include <QPointer>

namespace AzToolsFramework
{
    namespace ViewportInteraction
    {
        struct MouseInteractionEvent;
    }

    //! State of when and where the right-click context menu should appear.
    struct EditorContextMenu final
    {
        QPoint m_clickPoint = QPoint(0, 0);
        QPointer<QMenu> m_menu;
    };

    //! Update to run for context menu (when should it appear/disappear etc).
    void EditorContextMenuUpdate(EditorContextMenu& contextMenu, const ViewportInteraction::MouseInteractionEvent& mouseInteraction);
} // namespace AzToolsFramework
