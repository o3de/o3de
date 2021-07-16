/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/EditMenuActions/EditActionsMenuGroup.h>

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/EditMenuActions/EditContextMenuAction.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/EditMenuActions/EditContextMenuActions.h>

namespace GraphCanvas
{
    /////////////////////////
    // EditActionsMenuGroup
    /////////////////////////
    
    EditActionsMenuGroup::EditActionsMenuGroup()
        : m_cutAction(nullptr)
        , m_copyAction(nullptr)
        , m_pasteAction(nullptr)
        , m_deleteAction(nullptr)
        , m_duplicateAction(nullptr)
    {
    }
    
    EditActionsMenuGroup::~EditActionsMenuGroup()
    {
    }

    void EditActionsMenuGroup::PopulateMenu(EditorContextMenu* contextMenu)
    {
        contextMenu->AddActionGroup(EditContextMenuAction::GetEditContextMenuActionGroupId());

        m_cutAction = aznew CutGraphSelectionMenuAction(contextMenu);
        contextMenu->AddMenuAction(m_cutAction);

        m_copyAction = aznew CopyGraphSelectionMenuAction(contextMenu);
        contextMenu->AddMenuAction(m_copyAction);

        m_pasteAction = aznew PasteGraphSelectionMenuAction(contextMenu);
        contextMenu->AddMenuAction(m_pasteAction);

        m_duplicateAction = aznew DuplicateGraphSelectionMenuAction(contextMenu);
        contextMenu->AddMenuAction(m_duplicateAction);

        m_deleteAction = aznew DeleteGraphSelectionMenuAction(contextMenu);
        contextMenu->AddMenuAction(m_deleteAction);
    }
    
    void EditActionsMenuGroup::SetCutEnabled(bool enabled)
    {
        m_cutAction->setEnabled(enabled);
    }
    
    void EditActionsMenuGroup::SetCopyEnabled(bool enabled)
    {
        m_copyAction->setEnabled(enabled);
    }
    
    void EditActionsMenuGroup::SetPasteEnabled(bool enabled)
    {
        m_pasteAction->setEnabled(enabled);
    }
    
    void EditActionsMenuGroup::SetDeleteEnabled(bool enabled)
    {
        m_deleteAction->setEnabled(enabled);
    }
    
    void EditActionsMenuGroup::SetDuplicateEnabled(bool enabled)
    {
        m_duplicateAction->setEnabled(enabled);
    }
}
