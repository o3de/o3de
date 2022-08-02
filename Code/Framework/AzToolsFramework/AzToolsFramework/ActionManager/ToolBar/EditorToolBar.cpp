/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ActionManager/ToolBar/EditorToolBar.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/ActionManager/ToolBar/ToolBarManagerInterface.h>

#include <QMenu>
#include <QToolBar>
#include <QToolButton>

namespace AzToolsFramework
{
    EditorToolBar::EditorToolBar()
        : m_toolBar(new QToolBar("", m_defaultParentWidget))
    {
        m_toolBar->setMovable(false);
    }

    EditorToolBar::EditorToolBar(const AZStd::string& name)
        : m_toolBar(new QToolBar(name.c_str(), m_defaultParentWidget))
    {
        m_toolBar->setMovable(false);
    }

    void EditorToolBar::AddSeparator(int sortKey)
    {
        m_toolBarItems.insert({ sortKey, ToolBarItem(m_toolBar) });
    }
    
    void EditorToolBar::AddAction(int sortKey, AZStd::string actionIdentifier)
    {
        if (!m_actionToSortKeyMap.contains(actionIdentifier))
        {
            m_actionToSortKeyMap.insert(AZStd::make_pair(actionIdentifier, sortKey));
            m_toolBarItems.insert({ sortKey, ToolBarItem(m_toolBar, ToolBarItemType::Action, AZStd::move(actionIdentifier)) });
        }
    }

    void EditorToolBar::AddActionWithSubMenu(int sortKey, AZStd::string actionIdentifier, const AZStd::string& subMenuIdentifier)
    {
        if (!m_actionToSortKeyMap.contains(actionIdentifier))
        {
            m_actionToSortKeyMap.insert(AZStd::make_pair(actionIdentifier, sortKey));
            m_toolBarItems.insert(
                {
                    sortKey,
                    ToolBarItem(
                        m_toolBar, 
                        ToolBarItemType::ActionAndSubMenu,
                        AZStd::move(actionIdentifier),
                        AZStd::move(subMenuIdentifier))
                }
            );
        }
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

    void EditorToolBar::AddWidget(int sortKey, AZStd::string widgetActionIdentifier)
    {
        if (!m_widgetToSortKeyMap.contains(widgetActionIdentifier))
        {
            m_widgetToSortKeyMap.insert(AZStd::make_pair(widgetActionIdentifier, sortKey));
            m_toolBarItems.insert({ sortKey, ToolBarItem(m_toolBar, ToolBarItemType::Widget, AZStd::move(widgetActionIdentifier)) });
        }
    }

    bool EditorToolBar::ContainsAction(const AZStd::string& actionIdentifier) const
    {
        return m_actionToSortKeyMap.contains(actionIdentifier);
    }

    bool EditorToolBar::ContainsWidget(const AZStd::string& widgetActionIdentifier) const
    {
        return m_widgetToSortKeyMap.contains(widgetActionIdentifier);
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

    AZStd::optional<int> EditorToolBar::GetWidgetSortKey(const AZStd::string& widgetActionIdentifier) const
    {
        auto widgetIterator = m_widgetToSortKeyMap.find(widgetActionIdentifier);
        if (widgetIterator == m_widgetToSortKeyMap.end())
        {
            return AZStd::nullopt;
        }

        return widgetIterator->second;
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
                    if (QAction* action = m_actionManagerInternalInterface->GetAction(elem.second.m_identifier))
                    {
                        if (!action->isEnabled() && m_actionManagerInternalInterface->GetHideFromToolBarsWhenDisabled(elem.second.m_identifier))
                        {
                            continue;
                        }

                        m_toolBar->addAction(action);
                    }
                }
                break;
            case ToolBarItemType::Separator:
                {
                    m_toolBar->addSeparator();
                }
                break;
            case ToolBarItemType::ActionAndSubMenu:
            case ToolBarItemType::Widget:
                {
                    m_toolBar->addAction(elem.second.m_widgetAction);
                }
                break;
            default:
                break;
            }
        }
    }

    EditorToolBar::ToolBarItem::ToolBarItem(QToolBar* toolBar, ToolBarItemType type, AZStd::string identifier, AZStd::string subMenuIdentifier)
        : m_type(type)
        , m_identifier(AZStd::move(identifier))
        , m_subMenuIdentifier(AZStd::move(subMenuIdentifier))
    {
        switch (m_type)
        {
            case ToolBarItemType::Widget:
                {
                    if (QWidget* widget = m_actionManagerInternalInterface->GenerateWidgetFromWidgetAction(m_identifier))
                    {
                        m_widgetAction = new QWidgetAction(nullptr);
                        m_widgetAction->setDefaultWidget(widget);
                    }
                }
                break;

            case ToolBarItemType::ActionAndSubMenu:
                {
                    QAction* action = m_actionManagerInternalInterface->GetAction(m_identifier);
                    QMenu* subMenu = m_menuManagerInternalInterface->GetMenu(m_subMenuIdentifier);

                    if (action && subMenu)
                    {
                        QToolButton* toolButton = new QToolButton(toolBar);

                        toolButton->setPopupMode(QToolButton::MenuButtonPopup);
                        toolButton->setAutoRaise(true);
                        toolButton->setMenu(subMenu);
                        toolButton->setDefaultAction(action);

                        m_widgetAction = new QWidgetAction(m_defaultParentWidget);
                        m_widgetAction->setDefaultWidget(toolButton);
                    }
                }
                break;

            case ToolBarItemType::Action:
            case ToolBarItemType::Separator:
            default:
                break;
        }
    }

    void EditorToolBar::Initialize(QWidget* defaultParentWidget)
    {
        m_defaultParentWidget = defaultParentWidget;

        m_actionManagerInterface = AZ::Interface<ActionManagerInterface>::Get();
        AZ_Assert(m_actionManagerInterface, "EditorToolBar - Could not retrieve instance of ActionManagerInterface");

        m_actionManagerInternalInterface = AZ::Interface<ActionManagerInternalInterface>::Get();
        AZ_Assert(m_actionManagerInternalInterface, "EditorToolBar - Could not retrieve instance of ActionManagerInternalInterface");

        m_menuManagerInterface = AZ::Interface<MenuManagerInterface>::Get();
        AZ_Assert(m_menuManagerInterface, "EditorToolBar - Could not retrieve instance of MenuManagerInterface");

        m_menuManagerInternalInterface = AZ::Interface<MenuManagerInternalInterface>::Get();
        AZ_Assert(m_menuManagerInternalInterface, "EditorToolBar - Could not retrieve instance of MenuManagerInternalInterface");

        m_toolBarManagerInterface = AZ::Interface<ToolBarManagerInterface>::Get();
        AZ_Assert(m_toolBarManagerInterface, "EditorToolBar - Could not retrieve instance of ToolBarManagerInterface");
    }

} // namespace AzToolsFramework
