/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

namespace WhiteBox
{
    void RequestEditSourceControl(const char* absoluteFilePath)
    {
        bool active = false;
        AzToolsFramework::SourceControlConnectionRequestBus::BroadcastResult(
            active, &AzToolsFramework::SourceControlConnectionRequestBus::Events::IsActive);

        if (active)
        {
            AzToolsFramework::SourceControlCommandBus::Broadcast(
                &AzToolsFramework::SourceControlCommandBus::Events::RequestEdit, absoluteFilePath, true,
                []([[maybe_unused]] bool success, [[maybe_unused]] AzToolsFramework::SourceControlFileInfo info)
                {
                });
        }
    }

    IEditor* GetIEditor()
    {
        IEditor* editor = nullptr;
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(editor, &AzToolsFramework::EditorRequests::GetEditor);
        return editor;
    }
} // namespace WhiteBox
