/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
