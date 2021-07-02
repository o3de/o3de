/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <QApplication>

namespace AzToolsFramework
{
    /// Gets the currently active window, or the Editor's main window if there is no active window.
    /// Can be used to guarantee that modal dialogs have a valid parent in every context.
    inline QWidget* GetActiveWindow()
    {
        QWidget* activeWindow = QApplication::activeWindow();
        if (activeWindow == nullptr)
        {
            AzToolsFramework::EditorRequestBus::BroadcastResult(activeWindow, &AzToolsFramework::EditorRequests::GetMainWindow);
        }

        return activeWindow;
    }
} // namespace AzToolsFramework

