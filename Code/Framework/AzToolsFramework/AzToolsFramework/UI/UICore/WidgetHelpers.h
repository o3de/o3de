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

