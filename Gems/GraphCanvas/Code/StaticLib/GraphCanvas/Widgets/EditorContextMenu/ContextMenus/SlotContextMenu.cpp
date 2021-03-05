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
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenus/SlotContextMenu.h>

#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/SlotMenuActions/SlotContextMenuAction.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/SlotMenuActions/SlotContextMenuActions.h>

namespace GraphCanvas
{
    ////////////////////
    // SlotContextMenu
    ////////////////////

    SlotContextMenu::SlotContextMenu(EditorId editorId, QWidget* parent)
        : EditorContextMenu(editorId, parent)
    {
        AddActionGroup(SlotContextMenuAction::GetSlotContextMenuActionGroupId());

        AddMenuAction(aznew RemoveSlotMenuAction(this));
        AddMenuAction(aznew ClearConnectionsMenuAction(this));
        AddMenuAction(aznew ResetToDefaultValueMenuAction(this));

        bool allowDataReferences = false;
        AssetEditorSettingsRequestBus::EventResult(allowDataReferences, editorId, &AssetEditorSettingsRequests::AllowDataReferenceSlots);

        if (allowDataReferences)
        {
            AddMenuAction(aznew ToggleReferenceStateAction(this));            
        }

        AddMenuAction(aznew PromoteToVariableAction(this));
    }
}