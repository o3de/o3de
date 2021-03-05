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

#pragma once

#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QPoint>
#include <QPointer>
#include <QMenu>

namespace AzToolsFramework
{
    namespace ViewportInteraction
    {
        struct MouseInteractionEvent;
    }

    /// State of when and where the right-click context menu should appear.
    struct EditorContextMenu final
    {
        bool m_shouldOpen = false;
        QPoint m_clickPoint = QPoint(0, 0);
        QPointer<QMenu> m_menu;
    };

    /// Update to run for context menu (when should it appear/disappear etc).
    void EditorContextMenuUpdate(
        EditorContextMenu& contextMenu,
        const ViewportInteraction::MouseInteractionEvent& mouseInteraction);
} // namespace AzToolsFramework