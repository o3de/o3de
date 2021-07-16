/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/AlignmentMenuActions/AlignmentActionsMenuGroup.h>

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/AlignmentMenuActions/AlignmentContextMenuActions.h>

namespace GraphCanvas
{
    //////////////////////////////
    // AlignmentActionsMenuGroup
    //////////////////////////////
    AlignmentActionsMenuGroup::AlignmentActionsMenuGroup()
        : m_alignTop(nullptr)
        , m_alignBottom(nullptr)
        , m_alignRight(nullptr)
        , m_alignLeft(nullptr)
    {
    }

    void AlignmentActionsMenuGroup::PopulateMenu(EditorContextMenu* contextMenu)
    {
        contextMenu->AddActionGroup(AlignmentContextMenuAction::GetAlignmentContextMenuActionGroupId());

        m_alignTop = aznew AlignSelectionMenuAction("Align top", GraphUtils::VerticalAlignment::Top, GraphUtils::HorizontalAlignment::None, contextMenu);
        m_alignTop->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Up));

        contextMenu->AddMenuAction(m_alignTop);

        m_alignBottom = aznew AlignSelectionMenuAction("Align bottom", GraphUtils::VerticalAlignment::Bottom, GraphUtils::HorizontalAlignment::None, contextMenu);
        m_alignBottom->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Down));

        contextMenu->AddMenuAction(m_alignBottom);

        m_alignLeft = aznew AlignSelectionMenuAction("Align left", GraphUtils::VerticalAlignment::None, GraphUtils::HorizontalAlignment::Left, contextMenu);
        m_alignLeft->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Left));

        contextMenu->AddMenuAction(m_alignLeft);

        m_alignRight = aznew AlignSelectionMenuAction("Align right", GraphUtils::VerticalAlignment::None, GraphUtils::HorizontalAlignment::Right, contextMenu);
        m_alignRight->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Right));

        contextMenu->AddMenuAction(m_alignRight);

        m_editorMenu = contextMenu;
    }

    void AlignmentActionsMenuGroup::SetEnabled(bool enabled)
    {
        if (m_editorMenu)
        {
            QMenu* menu = m_editorMenu->FindSubMenu(m_alignRight->GetSubMenuPath());
            menu->setEnabled(enabled);
        }
    }
}
