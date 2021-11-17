/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QWidgetAction>

#include <GraphCanvas/Widgets/EditorContextMenu/EditorContextMenu.h>

namespace GraphCanvas
{
    //////////////////////
    // EditorContextMenu
    //////////////////////
    
    EditorContextMenu::EditorContextMenu(EditorId editorId, QWidget* parent)
        : QMenu(parent)
        , m_editorId(editorId)
        , m_finalized(false)
        , m_isToolBarMenu(false)
    {
    }

    void EditorContextMenu::SetIsToolBarMenu(bool isToolBarMenu)
    {
        m_isToolBarMenu = isToolBarMenu;
    }

    bool EditorContextMenu::IsToolBarMenu() const
    {
        return m_isToolBarMenu;
    }

    EditorId EditorContextMenu::GetEditorId() const
    {
        return m_editorId;
    }
    
    void EditorContextMenu::AddActionGroup(const ActionGroupId& actionGroup)
    {
        AZ_Error("GraphCanvas", !m_finalized, "Trying to configure a Menu that has already been finalized");
        if (!m_finalized)
        {
            auto insertResult = m_actionGroups.insert(actionGroup);

            if (insertResult.second)
            {
                m_actionGroupOrdering.push_back(actionGroup);
            }
        }
    }

    void EditorContextMenu::AddMenuAction(QAction* contextMenuAction, MenuActionSection section)
    {
        AZ_Error("GraphCanvas", !m_finalized, "Trying to configure a Menu that has already been finalized.");
        if (!m_finalized)
        {
            // Add the specified menu action to the proper section of the context menu.
            // This allows the client to place their custom actions at the front of the menu, back of the menu,
            // or the default which is just in the order they are configured.
            switch (section)
            {
            case MenuActionSection::Front:
                m_unprocessedFrontActions.emplace_back(contextMenuAction);
                break;
            case MenuActionSection::Default:
                m_unprocessedActions.emplace_back(contextMenuAction);
                break;
            case MenuActionSection::Back:
                m_unprocessedBackActions.emplace_back(contextMenuAction);
                break;
            }
        }
        else
        {
            delete contextMenuAction;
        }
    }

    bool EditorContextMenu::IsFinalized() const
    {
        return m_finalized;
    }

    QMenu* EditorContextMenu::FindSubMenu(AZStd::string_view subMenuPath)
    {
        auto subMenuIter = m_subMenuMap.find(subMenuPath);

        if (subMenuIter != m_subMenuMap.end())
        {
            return subMenuIter->second;
        }

        return nullptr;
    }

    void EditorContextMenu::AddMenuActionFront(QAction* contextMenuAction)
    {
        AddMenuAction(contextMenuAction, MenuActionSection::Front);
    }

    void EditorContextMenu::AddMenuActionBack(QAction* contextMenuAction)
    {
        AddMenuAction(contextMenuAction, MenuActionSection::Back);
    }

    void EditorContextMenu::AddNodePaletteMenuAction(const NodePaletteConfig& config)
    {
        if (m_nodePalette)
        {
            AZ_Assert(false, "This EditorContextMenu already contains a Node Palette.");
            return;
        }

        m_nodePalette = aznew NodePaletteWidget(nullptr);
        m_nodePalette->setProperty("HasNoWindowDecorations", true);
        m_nodePalette->SetupNodePalette(config);

        if (m_userNodePaletteWidth > 0)
        {
            m_nodePalette->setFixedWidth(m_userNodePaletteWidth);
        }

        QWidgetAction* actionWidget = new QWidgetAction(this);
        actionWidget->setDefaultWidget(m_nodePalette);

        AddMenuActionBack(actionWidget);

        QObject::connect(this, &QMenu::aboutToShow, this, &EditorContextMenu::SetupDisplay);
        QObject::connect(m_nodePalette, &NodePaletteWidget::OnCreateSelection, this, &EditorContextMenu::HandleContextMenuSelection);
    }

    void EditorContextMenu::RefreshActions(const GraphId& graphId, const AZ::EntityId& targetMemberId)
    {
        if (!m_finalized)
        {
            ConstructMenu();
        }

        if (m_finalized)
        {
            for (QAction* action : actions())
            {
                ContextMenuAction* contextMenuAction = qobject_cast<ContextMenuAction*>(action);

                if (contextMenuAction)
                {
                    contextMenuAction->SetTarget(graphId, targetMemberId);
                }
            }

            for (const auto& mapPair : m_subMenuMap)
            {
                bool enableMenu = false;
                for (QAction* action : mapPair.second->actions())
                {
                    ContextMenuAction* contextMenuAction = qobject_cast<ContextMenuAction*>(action);

                    if (contextMenuAction)
                    {
                        contextMenuAction->SetTarget(graphId, targetMemberId);
                        enableMenu = enableMenu || contextMenuAction->isEnabled();
                    }
                }

                mapPair.second->setEnabled(enableMenu);
            }
        }

        if (m_nodePalette)
        {
            m_nodePalette->ResetSourceSlotFilter();
        }

        OnRefreshActions(graphId, targetMemberId);
    }

    void EditorContextMenu::showEvent(QShowEvent* showEvent)
    {
        ConstructMenu();
        QMenu::showEvent(showEvent);
    }

    const NodePaletteWidget* EditorContextMenu::GetNodePalette() const
    {
        return m_nodePalette;
    }

    void EditorContextMenu::ResetSourceSlotFilter()
    {
        if (m_nodePalette)
        {
            m_nodePalette->ResetSourceSlotFilter();
        }
    }

    void EditorContextMenu::FilterForSourceSlot(const GraphId& graphId, const AZ::EntityId& sourceSlotId)
    {
        if (m_nodePalette)
        {
            m_nodePalette->FilterForSourceSlot(graphId, sourceSlotId);
        }
    }

    void EditorContextMenu::SetupDisplay()
    {
        if (m_nodePalette)
        {
            if (!m_nodePalette->parent())
            {
                m_nodePalette->setParent(this);
            }

            m_nodePalette->ResetDisplay();
            m_nodePalette->FocusOnSearchFilter();
        }
    }

    void EditorContextMenu::HandleContextMenuSelection()
    {
        // Close the menu once an item in the Node Palette context menu has been selected
        close();
    }
    
    void EditorContextMenu::OnRefreshActions(const GraphId& graphId, const AZ::EntityId& targetMemberId)
    {
        AZ_UNUSED(graphId);
        AZ_UNUSED(targetMemberId);
    }

    void EditorContextMenu::keyPressEvent(QKeyEvent* keyEvent)
    {
        if (m_nodePalette && !m_nodePalette->hasFocus())
        {
            QMenu::keyPressEvent(keyEvent);
        }
    }

    void EditorContextMenu::ConstructMenu()
    {
        if (!m_finalized)
        {
            m_finalized = true;

            // Process the actions in order of their specified sections
            for (auto actions : { m_unprocessedFrontActions, m_unprocessedActions, m_unprocessedBackActions })
            {
                AddUnprocessedActions(actions);
            }
        }
    }

    void EditorContextMenu::AddUnprocessedActions(AZStd::vector<QAction*>& actions)
    {
        for (ActionGroupId currentGroup : m_actionGroupOrdering)
        {
            bool addedElement = false;
            auto actionIter = actions.begin();

            while (actionIter != actions.end())
            {
                ContextMenuAction* contextMenuAction = qobject_cast<ContextMenuAction*>((*actionIter));
                if (contextMenuAction)
                {
                    if (contextMenuAction->GetActionGroupId() == currentGroup)
                    {
                        addedElement = true;

                        if (contextMenuAction->IsInSubMenu())
                        {
                            AZStd::string menuString = contextMenuAction->GetSubMenuPath();

                            auto subMenuIter = m_subMenuMap.find(menuString);

                            if (subMenuIter == m_subMenuMap.end())
                            {
                                QMenu* menu = addMenu(menuString.c_str());

                                auto insertResult = m_subMenuMap.insert(AZStd::make_pair(menuString, menu));

                                subMenuIter = insertResult.first;
                            }

                            subMenuIter->second->addAction(contextMenuAction);
                        }
                        else
                        {
                            addAction(contextMenuAction);
                        }

                        actionIter = actions.erase(actionIter);
                        continue;
                    }
                }

                ++actionIter;
            }

            if (addedElement)
            {
                addSeparator();
            }
        }

        for (QAction* uncategorizedAction : actions)
        {
            addAction(uncategorizedAction);
        }

        actions.clear();
    }

#include <StaticLib/GraphCanvas/Widgets/EditorContextMenu/moc_EditorContextMenu.cpp>
}
