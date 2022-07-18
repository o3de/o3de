/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ActionManager/Menu/EditorMenu.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>

#include <QMenu>

namespace AzToolsFramework
{
    EditorMenu::EditorMenu()
        : m_menu(new QMenu(""))
    {
    }

    EditorMenu::EditorMenu(const AZStd::string& name)
        : m_menu(new QMenu(name.c_str()))
    {
    }

    void EditorMenu::AddSeparator(int sortKey)
    {
        m_menuItems.insert({ sortKey, MenuItem() });
    }
    
    void EditorMenu::AddAction(int sortKey, AZStd::string actionIdentifier)
    {
        if (ContainsAction(actionIdentifier))
        {
            return;
        }

        m_actionToSortKeyMap.insert(AZStd::make_pair(actionIdentifier, sortKey));
        m_menuItems.insert({ sortKey, MenuItem(MenuItemType::Action, AZStd::move(actionIdentifier)) });
    }

    void EditorMenu::RemoveAction(AZStd::string actionIdentifier)
    {
        auto sortKeyIterator = m_actionToSortKeyMap.find(actionIdentifier);
        if (sortKeyIterator == m_actionToSortKeyMap.end())
        {
            return;
        }

        int sortKey = sortKeyIterator->second;

        auto multimapIterator = m_menuItems.find(sortKey);
        if (multimapIterator == m_menuItems.end())
        {
            return;
        }

        while (multimapIterator->first == sortKey)
        {
            if (multimapIterator->second.m_identifier == actionIdentifier)
            {
                m_menuItems.erase(multimapIterator);
                return;
            }

            ++multimapIterator;
        }
    }

    void EditorMenu::AddSubMenu(int sortKey, AZStd::string menuIdentifier)
    {
        if (ContainsSubMenu(menuIdentifier))
        {
            return;
        }

        m_subMenuToSortKeyMap.insert(AZStd::make_pair(menuIdentifier, sortKey));
        m_menuItems.insert({ sortKey, MenuItem(MenuItemType::SubMenu, AZStd::move(menuIdentifier)) });
    }

    void EditorMenu::AddWidget(int sortKey, QWidget* widget)
    {
        m_menuItems.insert({ sortKey, MenuItem(widget) });
    }
    
    bool EditorMenu::ContainsAction(const AZStd::string& actionIdentifier) const
    {
        return m_actionToSortKeyMap.contains(actionIdentifier);
    }

    bool EditorMenu::ContainsSubMenu(const AZStd::string& menuIdentifier) const
    {
        return m_subMenuToSortKeyMap.contains(menuIdentifier);
    }

    AZStd::optional<int> EditorMenu::GetActionSortKey(const AZStd::string& actionIdentifier) const
    {
        auto actionIterator = m_actionToSortKeyMap.find(actionIdentifier);
        if (actionIterator == m_actionToSortKeyMap.end())
        {
            return AZStd::nullopt;
        }

        return actionIterator->second;
    }

    AZStd::optional<int> EditorMenu::GetSubMenuSortKey(const AZStd::string& menuIdentifier) const
    {
        auto menuIterator = m_subMenuToSortKeyMap.find(menuIdentifier);
        if (menuIterator == m_subMenuToSortKeyMap.end())
        {
            return AZStd::nullopt;
        }

        return menuIterator->second;
    }

    QMenu* EditorMenu::GetMenu()
    {
        return m_menu;
    }

    const QMenu* EditorMenu::GetMenu() const
    {
        return m_menu;
    }

    void EditorMenu::RefreshMenu()
    {
        m_menu->clear();

        for (const auto& elem : m_menuItems)
        {
            switch (elem.second.m_type)
            {
            case MenuItemType::Action:
                {
                    if (QAction* action = m_actionManagerInternalInterface->GetAction(elem.second.m_identifier))
                    {
                        if (!action->isEnabled() && m_actionManagerInternalInterface->GetHideFromMenusWhenDisabled(elem.second.m_identifier))
                        {
                            continue;
                        }

                        m_menu->addAction(action);
                    }
                    break;
                }
            case MenuItemType::SubMenu:
                {
                    if (QMenu* menu = m_menuManagerInternalInterface->GetMenu(elem.second.m_identifier))
                    {
                        m_menu->addMenu(menu);
                    }
                    break;
                }
            case MenuItemType::Separator:
                {
                    m_menu->addSeparator();
                    break;
                }
            case MenuItemType::Widget:
                {
                    m_menu->addAction(elem.second.m_widgetAction);
                    break;
                }
            default:
                break;
            }
        }
    }

    EditorMenu::MenuItem::MenuItem(MenuItemType type, AZStd::string identifier)
        : m_type(type)
    {
        if (type != MenuItemType::Separator)
        {
            m_identifier = AZStd::move(identifier);
        }
    }

    EditorMenu::MenuItem::MenuItem(QWidget* widget)
        : m_type(MenuItemType::Widget)
    {
        m_widgetAction = new QWidgetAction(widget->parent());
        m_widgetAction->setDefaultWidget(widget);
    }

    void EditorMenu::Initialize()
    {
        m_actionManagerInterface = AZ::Interface<ActionManagerInterface>::Get();
        AZ_Assert(m_actionManagerInterface, "EditorMenu - Could not retrieve instance of ActionManagerInterface");

        m_actionManagerInternalInterface = AZ::Interface<ActionManagerInternalInterface>::Get();
        AZ_Assert(m_actionManagerInternalInterface, "EditorMenu - Could not retrieve instance of ActionManagerInternalInterface");

        m_menuManagerInterface = AZ::Interface<MenuManagerInterface>::Get();
        AZ_Assert(m_menuManagerInterface, "EditorMenu - Could not retrieve instance of MenuManagerInterface");

        m_menuManagerInternalInterface = AZ::Interface<MenuManagerInternalInterface>::Get();
        AZ_Assert(m_menuManagerInternalInterface, "EditorMenu - Could not retrieve instance of MenuManagerInternalInterface");
    }

} // namespace AzToolsFramework
