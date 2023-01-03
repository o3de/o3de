/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ActionManager/ToolBar/EditorToolBar.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInternalInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInternalInterface.h>
#include <AzToolsFramework/ActionManager/ToolBar/ToolBarManagerInterface.h>

#include <QMenu>
#include <QToolBar>
#include <QToolButton>

namespace AzToolsFramework
{
    EditorToolBar::EditorToolBar()
        : m_toolBar(new QToolBar("", s_defaultParentWidget))
    {
        m_toolBar->setMovable(false);
    }

    EditorToolBar::EditorToolBar(const AZStd::string& name)
        : m_toolBar(new QToolBar(name.c_str(), s_defaultParentWidget))
    {
        m_toolBar->setMovable(false);
    }

    void EditorToolBar::AddSeparator(int sortKey)
    {
        m_toolBarItems[sortKey].emplace_back();
    }
    
    void EditorToolBar::AddAction(int sortKey, AZStd::string actionIdentifier)
    {
        m_actionToSortKeyMap.insert(AZStd::make_pair(actionIdentifier, sortKey));
        m_toolBarItems[sortKey].emplace_back(ToolBarItemType::Action, AZStd::move(actionIdentifier));
    }

    void EditorToolBar::AddActionWithSubMenu(int sortKey, AZStd::string actionIdentifier, const AZStd::string& subMenuIdentifier)
    {
        if (!m_actionToSortKeyMap.contains(actionIdentifier))
        {
            m_actionToSortKeyMap.insert(AZStd::make_pair(actionIdentifier, sortKey));
            m_toolBarItems[sortKey].emplace_back(ToolBarItemType::ActionAndSubMenu, AZStd::move(actionIdentifier), AZStd::move(subMenuIdentifier));
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
        bool removed = false;

        AZStd::erase_if(
            m_toolBarItems[sortKey],
            [&](const ToolBarItem& item)
            {
                removed = true;
                return item.m_identifier == actionIdentifier;
            }
        );

        if (removed)
        {
            m_actionToSortKeyMap.erase(actionIdentifier);
        }
    }

    void EditorToolBar::AddWidget(int sortKey, AZStd::string widgetActionIdentifier)
    {
        if (!m_widgetToSortKeyMap.contains(widgetActionIdentifier))
        {
            m_widgetToSortKeyMap.insert(AZStd::make_pair(widgetActionIdentifier, sortKey));
            m_toolBarItems[sortKey].emplace_back(ToolBarItemType::Widget, AZStd::move(widgetActionIdentifier));
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

        for (const auto& vectorIterator : m_toolBarItems)
        {
            for (const auto& toolBarItem : vectorIterator.second)
            {
                switch (toolBarItem.m_type)
                {
                    case ToolBarItemType::Action:
                        {
                            if (QAction* action = s_actionManagerInternalInterface->GetAction(toolBarItem.m_identifier))
                            {
                                if (!action->isEnabled() &&
                                    s_actionManagerInternalInterface->GetHideFromToolBarsWhenDisabled(toolBarItem.m_identifier))
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
                            m_toolBar->addAction(toolBarItem.m_widgetAction);
                        }
                        break;
                    default:
                        break;
                }
            }
        }
    }

    EditorToolBar::ToolBarItem::ToolBarItem(ToolBarItemType type, AZStd::string identifier, AZStd::string subMenuIdentifier)
        : m_type(type)
        , m_identifier(AZStd::move(identifier))
        , m_subMenuIdentifier(AZStd::move(subMenuIdentifier))
    {
        switch (m_type)
        {
            case ToolBarItemType::Widget:
                {
                    if (QWidget* widget = s_actionManagerInternalInterface->GenerateWidgetFromWidgetAction(m_identifier))
                    {
                        m_widgetAction = new QWidgetAction(nullptr);
                        m_widgetAction->setDefaultWidget(widget);
                    }
                }
                break;

            case ToolBarItemType::ActionAndSubMenu:
                {
                    QAction* action = s_actionManagerInternalInterface->GetAction(m_identifier);
                    QMenu* subMenu = s_menuManagerInternalInterface->GetMenu(m_subMenuIdentifier);

                    if (action && subMenu)
                    {
                        QToolButton* toolButton = new QToolButton(s_defaultParentWidget);

                        toolButton->setPopupMode(QToolButton::MenuButtonPopup);
                        toolButton->setAutoRaise(true);
                        toolButton->setMenu(subMenu);
                        toolButton->setDefaultAction(action);

                        m_widgetAction = new QWidgetAction(s_defaultParentWidget);
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
        s_defaultParentWidget = defaultParentWidget;

        s_actionManagerInterface = AZ::Interface<ActionManagerInterface>::Get();
        AZ_Assert(s_actionManagerInterface, "EditorToolBar - Could not retrieve instance of ActionManagerInterface");

        s_actionManagerInternalInterface = AZ::Interface<ActionManagerInternalInterface>::Get();
        AZ_Assert(s_actionManagerInternalInterface, "EditorToolBar - Could not retrieve instance of ActionManagerInternalInterface");

        s_menuManagerInterface = AZ::Interface<MenuManagerInterface>::Get();
        AZ_Assert(s_menuManagerInterface, "EditorToolBar - Could not retrieve instance of MenuManagerInterface");

        s_menuManagerInternalInterface = AZ::Interface<MenuManagerInternalInterface>::Get();
        AZ_Assert(s_menuManagerInternalInterface, "EditorToolBar - Could not retrieve instance of MenuManagerInternalInterface");

        s_toolBarManagerInterface = AZ::Interface<ToolBarManagerInterface>::Get();
        AZ_Assert(s_toolBarManagerInterface, "EditorToolBar - Could not retrieve instance of ToolBarManagerInterface");
    }

    void EditorToolBar::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ToolBarItem>()
                ->Field("Type", &ToolBarItem::m_type)
                ->Field("Identifier", &ToolBarItem::m_identifier)
                ->Field("SubMenu", &ToolBarItem::m_subMenuIdentifier);

            serializeContext->Class<EditorToolBar>()
                ->Field("Items", &EditorToolBar::m_toolBarItems);
        }
    }

} // namespace AzToolsFramework
