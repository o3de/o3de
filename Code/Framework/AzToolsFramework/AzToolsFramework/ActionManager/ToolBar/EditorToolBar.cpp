/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ActionManager/ToolBar/EditorToolBar.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/ToolBar/ToolBarManagerInterface.h>

#include <QToolBar>

namespace AzToolsFramework
{
    EditorToolBar::EditorToolBar()
        : m_toolBar(new QToolBar(""))
    {
    }

    EditorToolBar::EditorToolBar(const AZStd::string& name)
        : m_toolBar(new QToolBar(name.c_str()))
    {
    }

    void EditorToolBar::AddSeparator(int sortKey)
    {
        m_toolBarItems.insert({ sortKey, ToolBarItem() });
    }
    
    void EditorToolBar::AddAction(int sortKey, AZStd::string actionIdentifier)
    {
        m_actionToSortKeyMap.insert(AZStd::make_pair(actionIdentifier, sortKey));
        m_toolBarItems.insert({ sortKey, ToolBarItem(ToolBarItemType::Action, AZStd::move(actionIdentifier)) });
    }

    void EditorToolBar::RemoveAction(AZStd::string actionIdentifier)
    {
        auto sortKeyIterator = m_actionToSortKeyMap.find(actionIdentifier);
        if (sortKeyIterator == m_actionToSortKeyMap.end())
        {
            return;
        }

        int sortKey = sortKeyIterator->second;

        auto multimapIterator = m_toolBarItems.find(sortKey);
        if (multimapIterator == m_toolBarItems.end())
        {
            return;
        }

        while (multimapIterator->first == sortKey)
        {
            if (multimapIterator->second.m_identifier == actionIdentifier)
            {
                m_toolBarItems.erase(multimapIterator);
                return;
            }

            ++multimapIterator;
        }
    }

    void EditorToolBar::AddWidget(int sortKey, QWidget* widget)
    {
        m_toolBarItems.insert({ sortKey, ToolBarItem(widget) });
    }

    bool EditorToolBar::ContainsAction(const AZStd::string& actionIdentifier) const
    {
        return m_actionToSortKeyMap.contains(actionIdentifier);
    }

    AZStd::optional<int> EditorToolBar::GetActionSortKey(const AZStd::string& actionIdentifier) const
    {
        auto actionIterator = m_actionToSortKeyMap.find(actionIdentifier);
        if (actionIterator == m_actionToSortKeyMap.end())
        {
            return AZStd::nullopt;
        }

        return actionIterator->second;
    }

    QToolBar* EditorToolBar::GetToolBar()
    {
        return m_toolBar;
    }

    const QToolBar* EditorToolBar::GetToolBar() const
    {
        return m_toolBar;
    }

    void EditorToolBar::RefreshToolBar()
    {
        m_toolBar->clear();

        for (const auto& elem : m_toolBarItems)
        {
            switch (elem.second.m_type)
            {
            case ToolBarItemType::Action:
                {
                    if(QAction* action = m_actionManagerInterface->GetAction(elem.second.m_identifier))
                    {
                        m_toolBar->addAction(action);
                    }
                    break;
                }
            case ToolBarItemType::Separator:
                {
                    m_toolBar->addSeparator();
                    break;
                }
            case ToolBarItemType::Widget:
                {
                    m_toolBar->addAction(elem.second.m_widgetAction);
                    break;
                }
            default:
                break;
            }
        }
    }

    EditorToolBar::ToolBarItem::ToolBarItem(ToolBarItemType type, AZStd::string identifier)
        : m_type(type)
    {
        if (type != ToolBarItemType::Separator)
        {
            m_identifier = AZStd::move(identifier);
        }
    }

    EditorToolBar::ToolBarItem::ToolBarItem(QWidget* widget)
        : m_type(ToolBarItemType::Widget)
    {
        m_widgetAction = new QWidgetAction(widget->parent());
        m_widgetAction->setDefaultWidget(widget);
    }

    void EditorToolBar::Initialize()
    {
        m_actionManagerInterface = AZ::Interface<ActionManagerInterface>::Get();
        AZ_Assert(m_actionManagerInterface, "EditorToolBar - Could not retrieve instance of ActionManagerInterface");

        m_toolBarManagerInterface = AZ::Interface<ToolBarManagerInterface>::Get();
        AZ_Assert(m_toolBarManagerInterface, "EditorToolBar - Could not retrieve instance of ToolBarManagerInterface");
    }

} // namespace AzToolsFramework
